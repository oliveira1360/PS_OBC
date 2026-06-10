plugins {
    kotlin("jvm")           version "2.0.21"
    kotlin("plugin.spring") version "2.0.21"
    kotlin("plugin.jpa")    version "2.0.21"
    id("org.springframework.boot")         version "3.4.5"
    id("io.spring.dependency-management") version "1.1.7"
}

group   = "org.example"
version = "1.0-SNAPSHOT"

repositories { mavenCentral() }

dependencies {
    // ── Core Spring ─────────────────────────────────────────────────────────
    implementation("org.springframework.boot:spring-boot-starter-web")
    implementation("org.springframework.boot:spring-boot-starter-websocket")
    implementation("org.springframework.boot:spring-boot-starter-actuator")

    // ── Persistence ─────────────────────────────────────────────────────────
    implementation("org.springframework.boot:spring-boot-starter-data-jpa")
    runtimeOnly("org.postgresql:postgresql")

    // ── Security / JWT ──────────────────────────────────────────────────────
    implementation("org.springframework.boot:spring-boot-starter-security")
    implementation("io.jsonwebtoken:jjwt-api:0.12.6")
    runtimeOnly("io.jsonwebtoken:jjwt-impl:0.12.6")
    runtimeOnly("io.jsonwebtoken:jjwt-jackson:0.12.6")

    // ── Kotlin ──────────────────────────────────────────────────────────────
    implementation("com.fasterxml.jackson.module:jackson-module-kotlin")
    implementation("org.jetbrains.kotlin:kotlin-reflect")

    // ── Serial / Bluetooth ──────────────────────────────────────────────────
    implementation("com.fazecast:jSerialComm:2.11.0")

    // ── Dev / Test ──────────────────────────────────────────────────────────
    developmentOnly("org.springframework.boot:spring-boot-devtools")
    testImplementation("org.springframework.boot:spring-boot-starter-test")
    testImplementation(kotlin("test"))
}

kotlin {
    jvmToolchain(21)
    compilerOptions { freeCompilerArgs.addAll("-Xjsr305=strict") }
}

tasks.withType<Test> { useJUnitPlatform() }
