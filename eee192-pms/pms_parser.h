// pms_parser.h

#ifndef PMS_PARSER_H
#define PMS_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // For size_t

struct prog_state_type; // Forward declaration

// --- PMS Packet Definitions ---
#define PMS_PACKET_START_BYTE_1         0x42
#define PMS_PACKET_START_BYTE_2         0x4D
#define PMS_PACKET_MAX_LENGTH           32
// #define PMS_ASCII_PAIR_BUFFER_LEN       2 // No longer needed

// --- Parsed PMS Data Structure ---
typedef struct {
    uint16_t pm1_0_std;
    uint16_t pm2_5_std;
    uint16_t pm10_std;
    uint16_t pm1_0_atm;
    uint16_t pm2_5_atm;
    uint16_t pm10_atm;
    uint16_t particles_0_3um;
    uint16_t particles_0_5um;
    uint16_t particles_1_0um;
    uint16_t particles_2_5um;
    uint16_t particles_5_0um;
    uint16_t particles_10um;
} pms_data_t;

// --- Parser Status Enum ---
typedef enum {
    PMS_PARSER_OK,
    PMS_PARSER_PROCESSING_BYTE,     // Byte is being processed
    PMS_PARSER_PACKET_INCOMPLETE,
    PMS_PARSER_CHECKSUM_ERROR,
    PMS_PARSER_INVALID_START_BYTES,
    PMS_PARSER_INVALID_LENGTH,
    PMS_PARSER_BUFFER_OVERFLOW
    // PMS_PARSER_NEED_MORE_CHARS, PMS_PARSER_INVALID_HEX_CHAR removed
} pms_parser_status_t;

// --- Internal Parser State ---
typedef enum {
    PMS_STATE_WAITING_FOR_START_BYTE_1,
    PMS_STATE_WAITING_FOR_START_BYTE_2,
    PMS_STATE_READING_LENGTH_HIGH,
    PMS_STATE_READING_LENGTH_LOW,
    PMS_STATE_READING_DATA,
    PMS_STATE_READING_CHECKSUM_HIGH,
    PMS_STATE_READING_CHECKSUM_LOW
} pms_parsing_state_e;

typedef struct {
    // ASCII conversion buffers removed
    // char ascii_char_pair[PMS_ASCII_PAIR_BUFFER_LEN];
    // uint8_t ascii_char_pair_idx;

    uint8_t packet_buffer[PMS_PACKET_MAX_LENGTH];
    uint8_t packet_buffer_idx;

    pms_parsing_state_e state;
    uint16_t expected_payload_len;
    uint16_t calculated_checksum;
} pms_parser_internal_state_t;

// --- Public Function Prototypes ---
void pms_parser_init(pms_parser_internal_state_t *state);

/**
 * @brief Feeds a single binary byte from the PMS sensor to the parser.
 *
 * @param ps Pointer to the main program state (for debug printing).
 * @param state Pointer to the pms_parser_internal_state_t structure.
 * @param byte The binary byte to feed.
 * @param out_data Pointer to a pms_data_t structure where parsed data will be stored
 *                 if a complete packet is successfully parsed.
 * @return pms_parser_status_t indicating the result of processing the byte.
 *         PMS_PARSER_OK means a packet was parsed and out_data is valid.
 */
pms_parser_status_t pms_parser_feed_byte(struct prog_state_type *ps,
                                         pms_parser_internal_state_t *state,
                                         uint8_t byte,
                                         pms_data_t *out_data);

#endif // PMS_PARSER_H