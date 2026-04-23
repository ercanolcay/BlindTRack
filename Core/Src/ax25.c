/*
  ******************************************************************************
  * @file           : ax25.c
  * @brief          : Ercan OLCAY # BlueTRace APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#include "ax25.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h> // for abs() function


// APRS SETTINGS
#define FLAG 0x7E
#define PREAMBLE_LEN 160
#define POSTAMBLE_LEN 10

static uint8_t frame[256];
static uint16_t frame_len = 0;

// Bitstream state
static uint16_t byte_p = 0;
static uint8_t bit_p = 0;
static uint8_t ones_count = 0;
static uint8_t nrzi_state = 1;
static uint8_t sending = 0;
static uint8_t stuff_pending = 0;

static uint16_t crc;

// ---------------- CRC ----------------
static void update_crc(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        uint8_t bit = (data >> i) & 1;
        uint16_t mix = (crc ^ bit) & 1;
        crc >>= 1;
        if (mix) crc ^= 0x8408;
    }
}

// ---------------- ADDRESS ----------------
static void write_address(const char *call, uint8_t ssid, uint8_t last) {
    int len = strlen(call);
    for (int i = 0; i < 6; i++) {
        char c = (i < len) ? call[i] : ' '; // If it's less than 6, put a space.
        frame[frame_len++] = c << 1;
    }
    frame[frame_len++] = (ssid << 1) | 0x60 | (last ? 1 : 0);
}

// ---------------- APRS FORMAT ----------------
static void format_latlon(float lat, float lon, char *out, char table_id) {
    char lat_dir = (lat >= 0.0f) ? 'N' : 'S';
    char lon_dir = (lon >= 0.0f) ? 'E' : 'W';

    float lat_abs = (lat < 0.0f) ? -lat : lat;
    float lon_abs = (lon < 0.0f) ? -lon : lon;

    int lat_deg = (int)(lat_abs / 100.0f);
    float lat_min = lat_abs - (lat_deg * 100.0f);

    int lon_deg = (int)(lon_abs / 100.0f);
    float lon_min = lon_abs - (lon_deg * 100.0f);

    // table_id variable is added
    sprintf(out, "%02d%05.2f%c%c%03d%05.2f%c",
            lat_deg, lat_min, lat_dir, table_id,
            lon_deg, lon_min, lon_dir);
}

// ---------------- FRAME BUILD ----------------
void Ax25WriteTrackerFrame(float lat, float lon, float speed, float course) {

    frame_len = 0;
    crc = 0xFFFF;

    // state reset
    byte_p = 0; bit_p = 0; ones_count = 0; nrzi_state = 1; stuff_pending = 0;

    // 1. PREAMBLE
    for (int i = 0; i < PREAMBLE_LEN; i++) frame[frame_len++] = FLAG;

    // 2. HEADER (AX.25 MANDATORY ARRANGEMENT)
    // The order MUST be like this: DEST, SOURCE, DIGI1, DIGI2(last)
    write_address("APRS", 0, 0);                                  // 1. TARGET
    write_address(device_config.callsign, device_config.ssid, 0); // 2. SOURCE
    write_address("WIDE1", 1, 0);                                 // 3. PATH 1
    write_address("WIDE2", 1, 1);                                 // 4. PATH 2 (last = 1)

    frame[frame_len++] = 0x03; // Control
    frame[frame_len++] = 0xF0; // PID

    // 3. PAYLOAD
    char payload[100];
    char pos[50];

    format_latlon(lat, lon, pos, device_config.table_id);

    // Yön ve Hız hesaplama
    int cse = (int)course % 360;
    if (cse < 0) cse += 360;
    int spd = (int)speed;

    // PROTOCOL STRUCTURE:
    // '!' -> Start of packet without timestamp (removes those strange numbers)
    // '%s' -> Coordinate (pos)
    // '/' -> Table Identifier (Primary)
    // '[' -> Symbol Identifier (the satellite bus icon you want)
    // '%03d/%03d' -> Direction and Speed (3-digit direction / 3-digit speed in knots)
    // ' ' -> Space (comment begins after this)

    sprintf(payload, "!%s%c%03u/%03u %s",
            pos,
            device_config.symbol,
            (unsigned int)cse,
            (unsigned int)spd,
            device_config.comment);

    for (int i = 0; i < strlen(payload); i++)
        frame[frame_len++] = payload[i];

    // 4. CRC VE 5. POSTAMBLE
    for (int i = PREAMBLE_LEN; i < frame_len; i++) update_crc(frame[i]);
    uint16_t final_crc = ~crc;
    frame[frame_len++] = final_crc & 0xFF;
    frame[frame_len++] = final_crc >> 8;
    for (int i = 0; i < POSTAMBLE_LEN; i++) frame[frame_len++] = FLAG;

    sending = 1;
}

// ---------------- BIT OUTPUT ----------------
uint8_t Ax25GetTxBit(void) {
    if (!sending) return 0xFF;
    if (stuff_pending) {
        stuff_pending = 0;
        nrzi_state ^= 1;
        return nrzi_state;
    } else {
        if (byte_p >= frame_len) {
            sending = 0;
            return 0xFF;
        }
        uint8_t current_byte = frame[byte_p];
        uint8_t current_bit = (current_byte >> bit_p) & 1;
        uint8_t is_flag = (current_byte == 0x7E);
        bit_p++;
        if (bit_p >= 8) {
            bit_p = 0;
            byte_p++;
        }
        if (!is_flag) {
            if (current_bit) {
                ones_count++;
                if (ones_count == 5) stuff_pending = 1;
            } else ones_count = 0;
        } else ones_count = 0;
        if (current_bit == 0) nrzi_state ^= 1;
    }
    return nrzi_state;
}

void Ax25Init(void) {
    sending = 0;
    frame_len = 0;
}

uint8_t Ax25IsSending(void) {
    return sending;
}
