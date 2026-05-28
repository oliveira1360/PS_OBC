/**
 * @file ota_verify.h
 * @brief Verificação CRC32 do firmware OTA.
 *
 * Implementação do CRC32 padrão (polinómio 0xEDB88320, ISO 3309 / ITU-T V.42).
 * Compatível com binutils crc32, zlib crc32, Python binascii.crc32.
 *
 * Uso típico no lado do ground station (Python):
 *   import binascii
 *   crc = binascii.crc32(firmware_bytes) & 0xFFFFFFFF
 *
 * O valor resultante deve ser gravado em ota_metadata_t.firmware_crc32.
 */

#ifndef OTA_VERIFY_H
#define OTA_VERIFY_H

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * API CRC32
 * ========================================================================= */

/**
 * @brief Calcula CRC32 de um bloco de dados.
 *
 * @param data  Ponteiro para os dados.
 * @param len   Número de bytes.
 * @return CRC32 calculado.
 */
uint32_t crc32_compute(const uint8_t *data, uint32_t len);

/**
 * @brief Actualiza um CRC32 parcial (para cálculo em blocos).
 *
 * Permite calcular o CRC32 de um stream dividido em múltiplos blocos:
 *   uint32_t crc = 0xFFFFFFFFUL;
 *   crc = crc32_update(crc, bloco1, len1);
 *   crc = crc32_update(crc, bloco2, len2);
 *   crc ^= 0xFFFFFFFFUL;  // finalização
 *
 * @param crc   CRC parcial anterior (0xFFFFFFFF para iniciar).
 * @param data  Bloco de dados.
 * @param len   Número de bytes no bloco.
 * @return CRC32 parcial actualizado.
 */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len);

/**
 * @brief Verifica o CRC32 de um firmware lido da flash externa.
 *
 * Lê o firmware em blocos de VERIFY_CHUNK_SIZE bytes directamente da
 * flash externa e calcula o CRC32 incrementalmente.
 *
 * @param ext_addr  Endereço na flash externa onde começa o firmware.
 * @param size      Tamanho do firmware em bytes.
 * @param expected  CRC32 esperado (de ota_metadata_t.firmware_crc32).
 * @return true se o CRC32 bate certo, false caso contrário.
 */
bool ota_verify_crc(uint32_t ext_addr, uint32_t size, uint32_t expected);

#endif /* OTA_VERIFY_H */
