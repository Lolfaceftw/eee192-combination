// pms_parser.c

#include "../../inc/parsers/pms_parser.h"
#include <string.h> // For memset
#include <stdio.h>

// Forward declaration from main.c for debug_printf
struct prog_state_type; // Already in pms_parser.h
void debug_printf(struct prog_state_type *ps, const char *format, ...);

// --- Static Helper Function Prototypes ---
// hex_char_to_int removed
static void _pms_parser_reset_packet_state(struct prog_state_type *ps, pms_parser_internal_state_t *state);
// _pms_process_byte is now pms_parser_feed_byte and public

// --- Public Function Implementations ---

void pms_parser_init(pms_parser_internal_state_t *state) {
    memset(state, 0, sizeof(pms_parser_internal_state_t));
    state->state = PMS_STATE_WAITING_FOR_START_BYTE_1;
    state->packet_buffer_idx = 0;
    state->calculated_checksum = 0;
    // state->ascii_char_pair_idx = 0; // Removed
}

// pms_parser_feed_ascii_char removed.
// _pms_process_byte is now the public pms_parser_feed_byte

// --- Static Helper Function Implementations ---
// hex_char_to_int removed

static void _pms_parser_reset_packet_state(struct prog_state_type *ps, pms_parser_internal_state_t *state) {
    debug_printf(ps, "Parser: Resetting packet state. Current state was %d\r\n", state->state);
    state->state = PMS_STATE_WAITING_FOR_START_BYTE_1;
    state->packet_buffer_idx = 0;
    state->calculated_checksum = 0;
    state->expected_payload_len = 0;
}

// This was _pms_process_byte, now it's the public API for feeding binary bytes
pms_parser_status_t pms_parser_feed_byte(struct prog_state_type *ps,
                                         pms_parser_internal_state_t *state,
                                         uint8_t byte,
                                         pms_data_t *out_data) {
    // debug_printf(ps, "Feed Byte: 0x%02X, State: %d, Idx: %d\r\n", byte, state->state, state->packet_buffer_idx);

    if (state->packet_buffer_idx >= PMS_PACKET_MAX_LENGTH &&
        (state->state != PMS_STATE_WAITING_FOR_START_BYTE_1)) {
        debug_printf(ps, "Parser ERR: Buffer overflow before reset. Idx: %d\r\n", state->packet_buffer_idx);
        _pms_parser_reset_packet_state(ps, state);
        return PMS_PARSER_BUFFER_OVERFLOW;
    }

    switch (state->state) {
        case PMS_STATE_WAITING_FOR_START_BYTE_1:
            if (byte == PMS_PACKET_START_BYTE_1) {
                state->packet_buffer[0] = byte;
                state->packet_buffer_idx = 1;
                state->calculated_checksum = byte;
                state->state = PMS_STATE_WAITING_FOR_START_BYTE_2;
                // debug_printf(ps, "Parser: Got SB1 (0x42)\r\n");
            } else {
                // debug_printf(ps, "Parser: Waiting SB1, got 0x%02X\r\n", byte);
            }
            break;

        case PMS_STATE_WAITING_FOR_START_BYTE_2:
            if (byte == PMS_PACKET_START_BYTE_2) {
                state->packet_buffer[state->packet_buffer_idx++] = byte;
                state->calculated_checksum += byte;
                state->state = PMS_STATE_READING_LENGTH_HIGH;
                // debug_printf(ps, "Parser: Got SB2 (0x4D)\r\n");
            } else {
                debug_printf(ps, "Parser ERR: Expected SB2 (0x4D), got 0x%02X. Resetting.\r\n", byte);
                _pms_parser_reset_packet_state(ps, state);
                if (byte == PMS_PACKET_START_BYTE_1) { // Check if this byte is a new start
                   return pms_parser_feed_byte(ps, state, byte, out_data); // Re-process this byte
                }
                return PMS_PARSER_INVALID_START_BYTES;
            }
            break;

        case PMS_STATE_READING_LENGTH_HIGH:
            state->packet_buffer[state->packet_buffer_idx++] = byte;
            state->calculated_checksum += byte;
            state->expected_payload_len = (uint16_t)byte << 8;
            state->state = PMS_STATE_READING_LENGTH_LOW;
            break;

        case PMS_STATE_READING_LENGTH_LOW:
            state->packet_buffer[state->packet_buffer_idx++] = byte;
            state->calculated_checksum += byte;
            state->expected_payload_len |= byte;
            debug_printf(ps, "Parser: Expected payload len = %u (0x%04X)\r\n", state->expected_payload_len, state->expected_payload_len);

            if (state->expected_payload_len == 0 ||
                (4 + state->expected_payload_len) > PMS_PACKET_MAX_LENGTH ||
                state->expected_payload_len < 2) { // Payload must be at least 2 for checksum
                debug_printf(ps, "Parser ERR: Invalid length %u. Resetting.\r\n", state->expected_payload_len);
                _pms_parser_reset_packet_state(ps, state);
                return PMS_PARSER_INVALID_LENGTH;
            }
            state->state = PMS_STATE_READING_DATA;
            break;

        case PMS_STATE_READING_DATA:
            state->packet_buffer[state->packet_buffer_idx++] = byte;
            if ((state->packet_buffer_idx -1) < (4 + state->expected_payload_len - 2)) {
                 state->calculated_checksum += byte;
            }

            if ((state->packet_buffer_idx - 4) == (state->expected_payload_len - 2)) {
                state->state = PMS_STATE_READING_CHECKSUM_HIGH;
            }
            break;

        case PMS_STATE_READING_CHECKSUM_HIGH:
            state->packet_buffer[state->packet_buffer_idx++] = byte;
            state->state = PMS_STATE_READING_CHECKSUM_LOW;
            break;

        case PMS_STATE_READING_CHECKSUM_LOW:
            state->packet_buffer[state->packet_buffer_idx++] = byte;
            
            uint16_t received_checksum = ((uint16_t)state->packet_buffer[state->packet_buffer_idx - 2] << 8) |
                                         state->packet_buffer[state->packet_buffer_idx - 1];

            debug_printf(ps, "Parser: Calc CS: 0x%04X, Recv CS: 0x%04X\r\n", state->calculated_checksum, received_checksum);

            if (state->calculated_checksum == received_checksum) {
                debug_printf(ps, "Parser: Checksum OK!\r\n");
                if (state->expected_payload_len >= 28) { 
                    out_data->pm1_0_std = ((uint16_t)state->packet_buffer[4] << 8) | state->packet_buffer[5];
                    out_data->pm2_5_std = ((uint16_t)state->packet_buffer[6] << 8) | state->packet_buffer[7];
                    out_data->pm10_std  = ((uint16_t)state->packet_buffer[8] << 8) | state->packet_buffer[9];
                    
                    out_data->pm1_0_atm = ((uint16_t)state->packet_buffer[10] << 8) | state->packet_buffer[11];
                    out_data->pm2_5_atm = ((uint16_t)state->packet_buffer[12] << 8) | state->packet_buffer[13];
                    out_data->pm10_atm  = ((uint16_t)state->packet_buffer[14] << 8) | state->packet_buffer[15];

                    out_data->particles_0_3um = ((uint16_t)state->packet_buffer[16] << 8) | state->packet_buffer[17];
                    out_data->particles_0_5um = ((uint16_t)state->packet_buffer[18] << 8) | state->packet_buffer[19];
                    out_data->particles_1_0um = ((uint16_t)state->packet_buffer[20] << 8) | state->packet_buffer[21];
                    out_data->particles_2_5um = ((uint16_t)state->packet_buffer[22] << 8) | state->packet_buffer[23];
                    out_data->particles_5_0um = ((uint16_t)state->packet_buffer[24] << 8) | state->packet_buffer[25];
                    out_data->particles_10um  = ((uint16_t)state->packet_buffer[26] << 8) | state->packet_buffer[27];
                } else {
                    debug_printf(ps, "Parser ERR: Packet too short (%u) for full parse, but CS OK.\r\n", state->expected_payload_len);
                    _pms_parser_reset_packet_state(ps, state);
                    return PMS_PARSER_INVALID_LENGTH; 
                }

                _pms_parser_reset_packet_state(ps, state);
                return PMS_PARSER_OK;
            } else {
                debug_printf(ps, "Parser ERR: Checksum mismatch. Resetting.\r\n");
                _pms_parser_reset_packet_state(ps, state);
                return PMS_PARSER_CHECKSUM_ERROR;
            }
            break;

        default:
            debug_printf(ps, "Parser ERR: Unknown state %d. Resetting.\r\n", state->state);
            _pms_parser_reset_packet_state(ps, state);
            break;
    }
    return PMS_PARSER_PROCESSING_BYTE;
}