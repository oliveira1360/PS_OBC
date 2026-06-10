-- =============================================================================
-- Satellite Dashboard — PostgreSQL Schema
-- Suporta: séries temporais, RBAC (roles/permissions), sessões de utilizadores
-- =============================================================================

-- Extensões
CREATE EXTENSION IF NOT EXISTS "pgcrypto";   -- gen_random_uuid()
CREATE EXTENSION IF NOT EXISTS "pg_trgm";    -- índices de texto parcial

-- ---------------------------------------------------------------------------
-- 1. RBAC — Roles & Permissões
-- ---------------------------------------------------------------------------

CREATE TABLE roles (
    id          SERIAL      PRIMARY KEY,
    name        VARCHAR(50) NOT NULL UNIQUE,   -- ADMIN, OPERATOR, VIEWER
    description TEXT
);

CREATE TABLE permissions (
    id          SERIAL       PRIMARY KEY,
    name        VARCHAR(100) NOT NULL UNIQUE,  -- ex: history:read, ota:write
    description TEXT
);

CREATE TABLE role_permissions (
    role_id       INT NOT NULL REFERENCES roles(id)       ON DELETE CASCADE,
    permission_id INT NOT NULL REFERENCES permissions(id) ON DELETE CASCADE,
    PRIMARY KEY (role_id, permission_id)
);

-- ---------------------------------------------------------------------------
-- 2. Utilizadores & Sessões
-- ---------------------------------------------------------------------------

CREATE TABLE users (
    id            UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    username      VARCHAR(50) NOT NULL UNIQUE,
    email         VARCHAR(150) NOT NULL UNIQUE,
    password_hash TEXT        NOT NULL,
    enabled       BOOLEAN     NOT NULL DEFAULT TRUE,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Mapeado como @ElementCollection na entidade UserEntity (role_name = string do enum Role)
CREATE TABLE user_roles (
    user_id   UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    role_name VARCHAR(50) NOT NULL,
    PRIMARY KEY (user_id, role_name)
);

CREATE TABLE sessions (
    id            UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id       UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token_hash    TEXT        NOT NULL UNIQUE,  -- SHA-256 do JWT
    ip_address    INET,
    user_agent    TEXT,
    expires_at    TIMESTAMPTZ NOT NULL,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    revoked_at    TIMESTAMPTZ                    -- NULL = activa
);

CREATE INDEX idx_sessions_user_id   ON sessions(user_id);
CREATE INDEX idx_sessions_token     ON sessions(token_hash);
CREATE INDEX idx_sessions_expires   ON sessions(expires_at);

-- ---------------------------------------------------------------------------
-- 2b. Convites (registo apenas por convite gerado por um ADMIN)
-- ---------------------------------------------------------------------------
-- Um ADMIN gera um código de 10 caracteres associado a um role. O novo
-- utilizador regista-se introduzindo esse código. Cada convite é de uso único
-- e expira (expires_at). Pode ser revogado manualmente (revoked_at).

CREATE TABLE invites (
    id          UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    code        VARCHAR(10) NOT NULL UNIQUE,        -- 10 caracteres aleatórios
    role_name   VARCHAR(50) NOT NULL,               -- role atribuído ao registar
    email       VARCHAR(150),                       -- opcional: email pré-associado
    created_by  VARCHAR(50) NOT NULL,               -- username do admin que criou
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    expires_at  TIMESTAMPTZ NOT NULL,               -- prazo de validade
    used_at     TIMESTAMPTZ,                        -- NULL = ainda não usado
    used_by     UUID        REFERENCES users(id) ON DELETE SET NULL,
    revoked_at  TIMESTAMPTZ                         -- NULL = não revogado
);

CREATE INDEX idx_invites_code ON invites(code);

-- ---------------------------------------------------------------------------
-- 3. Tipos de Sensores (catálogo)
-- ---------------------------------------------------------------------------

CREATE TABLE sensor_types (
    id          SERIAL       PRIMARY KEY,
    key         VARCHAR(50)  NOT NULL UNIQUE,  -- ex: temperature, voltage
    label       VARCHAR(100) NOT NULL,         -- nome legível
    unit        VARCHAR(20),                   -- ex: °C, V, hPa
    value_min   DOUBLE PRECISION,
    value_max   DOUBLE PRECISION,
    description TEXT
);

-- ---------------------------------------------------------------------------
-- 4. Leituras de Sensores (série temporal)
-- ---------------------------------------------------------------------------

-- sensor_key: chave directa do tipo de sensor (ex: temperature, voltage)
-- Evita joins em queries de séries temporais de alta frequência.
CREATE TABLE telemetry_readings (
    id            BIGSERIAL        PRIMARY KEY,
    recorded_at   TIMESTAMPTZ      NOT NULL DEFAULT NOW(),
    sensor_key    VARCHAR(50)      NOT NULL,   -- ex: temperature, voltage, accel_x
    value_numeric DOUBLE PRECISION,            -- Temperature, Voltage, Pressure…
    value_integer INTEGER,                     -- RSSI, battery level…
    source        VARCHAR(50),                 -- ex: COM3, BT, SIM
    quality       SMALLINT DEFAULT 100         -- 0-100 (qualidade do dado)
);

-- Índices por tempo e por sensor (queries de janela temporal)
CREATE INDEX idx_telemetry_recorded_at ON telemetry_readings(recorded_at DESC);
CREATE INDEX idx_telemetry_sensor_key  ON telemetry_readings(sensor_key, recorded_at DESC);

-- Particionamento mensal (PostgreSQL 11+) — opcional mas recomendado para produção
-- (descomentado permite particionamento automático)
-- ALTER TABLE telemetry_readings PARTITION BY RANGE (recorded_at);

-- ---------------------------------------------------------------------------
-- 5. Dados iniciais (seed)
-- ---------------------------------------------------------------------------

-- Roles
INSERT INTO roles (name, description) VALUES
    ('ADMIN',    'Acesso total: configurar, enviar OTA, gerir utilizadores'),
    ('OPERATOR', 'Operar o satélite: enviar comandos, ver histórico'),
    ('VIEWER',   'Apenas leitura: dashboard e histórico');

-- Permissões granulares
INSERT INTO permissions (name, description) VALUES
    ('dashboard:read',  'Ver dashboard em tempo real'),
    ('history:read',    'Consultar histórico de sensores'),
    ('history:export',  'Exportar histórico para CSV/JSON'),
    ('ota:write',       'Iniciar actualização OTA'),
    ('commands:write',  'Enviar comandos ao OBC'),
    ('users:manage',    'Criar/editar/remover utilizadores'),
    ('audit:read',      'Ver registo de auditoria');

-- Atribuição de permissões por role
-- ADMIN: tudo
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r, permissions p WHERE r.name = 'ADMIN';

-- OPERATOR: dashboard + histórico + comandos + OTA
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r
JOIN permissions p ON p.name IN ('dashboard:read','history:read','history:export','ota:write','commands:write')
WHERE r.name = 'OPERATOR';

-- VIEWER: só leitura
INSERT INTO role_permissions (role_id, permission_id)
SELECT r.id, p.id FROM roles r
JOIN permissions p ON p.name IN ('dashboard:read','history:read')
WHERE r.name = 'VIEWER';

-- Tipos de sensores (correspondem aos campos de TelemetryData)
INSERT INTO sensor_types (key, label, unit, value_min, value_max) VALUES
    ('temperature',   'Temperatura',          '°C',    -50,   125),
    ('pressure',      'Pressão',              'hPa',     0,  2000),
    ('voltage',       'Tensão EPS',           'V',       0,     8),
    ('current',       'Corrente EPS',         'A',       0,     5),
    ('battery_level', 'Nível de Bateria',     '%',       0,   100),
    ('latitude',      'Latitude GNSS',        '°',     -90,    90),
    ('longitude',     'Longitude GNSS',       '°',    -180,   180),
    ('altitude',      'Altitude GNSS',        'km',      0,  2000),
    ('speed',         'Velocidade Orbital',   'km/s',    0,    12),
    ('accel_x',       'Aceleração X',         'g',      -4,     4),
    ('accel_y',       'Aceleração Y',         'g',      -4,     4),
    ('accel_z',       'Aceleração Z',         'g',      -4,     4),
    ('gyro_x',        'Giroscópio X',         '°/s',  -250,   250),
    ('gyro_y',        'Giroscópio Y',         '°/s',  -250,   250),
    ('gyro_z',        'Giroscópio Z',         '°/s',  -250,   250),
    ('mag_x',         'Magnetómetro X',       'µT',   -100,   100),
    ('mag_y',         'Magnetómetro Y',       'µT',   -100,   100),
    ('mag_z',         'Magnetómetro Z',       'µT',   -100,   100),
    ('rssi',          'RSSI',                 'dBm',  -120,     0),
    ('doppler',       'Desvio Doppler',       'Hz',  -5000,  5000);

-- Utilizador admin por defeito (password: admin123 — TROCAR EM PRODUÇÃO)
INSERT INTO users (username, email, password_hash) VALUES
    ('admin', 'admin@satellite.local',
     '$2a$12$0DpMFhKfyX5/t5kzkD1MzOYrXwLDkf2uvfwXS1HGRuLNQboCp9La.');
-- Atribuir role ADMIN ao utilizador admin
INSERT INTO user_roles (user_id, role_name)
SELECT u.id, 'ADMIN' FROM users u WHERE u.username = 'admin';

-- ---------------------------------------------------------------------------
-- 6. Auditoria (quem fez o quê e quando)
-- ---------------------------------------------------------------------------

CREATE TABLE audit_log (
    id          BIGSERIAL    PRIMARY KEY,
    occurred_at TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    username    VARCHAR(50),                 -- NULL se desconhecido
    action      VARCHAR(60)  NOT NULL,       -- LOGIN_SUCCESS, COMMAND_SEND, OTA_UPLOAD…
    target      VARCHAR(200),                -- alvo da acção (comando, ficheiro, user…)
    details     TEXT,
    ip_address  VARCHAR(45),                 -- IPv4/IPv6
    success     BOOLEAN      NOT NULL DEFAULT TRUE
);

CREATE INDEX idx_audit_occurred ON audit_log(occurred_at DESC);
CREATE INDEX idx_audit_username ON audit_log(username);
CREATE INDEX idx_audit_action   ON audit_log(action);

-- ---------------------------------------------------------------------------
-- 7. Rastreio orbital (TLE + estação terrestre)
-- ---------------------------------------------------------------------------

CREATE TABLE satellite_tle (
    id         BIGSERIAL    PRIMARY KEY,
    name       VARCHAR(100),
    line1      VARCHAR(80)  NOT NULL,
    line2      VARCHAR(80)  NOT NULL,
    norad_id   VARCHAR(10),
    source     VARCHAR(20)  NOT NULL DEFAULT 'MANUAL',   -- MANUAL | CELESTRAK
    updated_at TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

CREATE TABLE ground_station (
    id         INT              PRIMARY KEY,
    name       VARCHAR(100)     NOT NULL,
    latitude   DOUBLE PRECISION NOT NULL,
    longitude  DOUBLE PRECISION NOT NULL,
    altitude_m DOUBLE PRECISION NOT NULL DEFAULT 0
);

-- Estação por defeito (ISEL, Lisboa). Editável pelo admin.
INSERT INTO ground_station (id, name, latitude, longitude, altitude_m) VALUES
    (1, 'ISEL — Lisboa', 38.7566, -9.1163, 80);
