/*
  ******************************************************************************
  * @file           : tracker.c
  * @brief          : Ercan OLCAY # BlindTRack APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#include "tracker.h"
#include "ptt.h"
#include "gps_handler.h"
#include "ax25.h"
#include "modem.h"
#include "config.h"
#include <math.h>

uint32_t last_tx     = 0xFFF00000;
static float last_course = 0.0f;  // Last transmitted heading (degrees)

// ── SmartBeaconing ────────────────────────────────────────────────────────────

// Returns the required beacon interval in milliseconds based on current speed/course
static uint32_t SmartBeacon_Interval(void) {
    float speed_kmh = gps_data.speed * 1.852f; // knots → km/h

    // Stopped or very slow — use slow rate
    if (speed_kmh < (float)device_config.sb_low_speed) {
        return (uint32_t)device_config.sb_slow_rate * 60000UL;
    }

    // Fast — use fast rate
    if (speed_kmh >= (float)device_config.sb_high_speed) {
        return (uint32_t)device_config.sb_fast_rate * 1000UL;
    }

    // Between low and high speed — interpolate linearly
    float ratio = (speed_kmh - (float)device_config.sb_low_speed) /
                  ((float)device_config.sb_high_speed - (float)device_config.sb_low_speed);

    uint32_t slow_ms = (uint32_t)device_config.sb_slow_rate * 60000UL;
    uint32_t fast_ms = (uint32_t)device_config.sb_fast_rate * 1000UL;

    return slow_ms - (uint32_t)(ratio * (float)(slow_ms - fast_ms));
}

// Returns 1 if turn angle exceeds threshold (corner pegging)
static uint8_t SmartBeacon_TurnDetected(void) {
    float speed_kmh = gps_data.speed * 1.852f;

    // Don't trigger turn beacon when stopped
    if (speed_kmh < 2.0f) return 0;

    // Turn threshold increases as speed decreases (sb_turn_slope / speed)
    float turn_threshold = (float)device_config.sb_turn_min +
                           (float)device_config.sb_turn_slope / speed_kmh;

    // Angle difference, wrapped to -180..+180
    float diff = gps_data.course - last_course;
    if (diff >  180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;
    if (diff < 0.0f)    diff = -diff;

    return (diff >= turn_threshold) ? 1 : 0;
}

// ── TX helper ─────────────────────────────────────────────────────────────────

static void Transmit_Packet(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    PTT_On();
    HAL_Delay(500); // AnyTone wake up
    Ax25WriteTrackerFrame(gps_data.lat, gps_data.lon, gps_data.speed, gps_data.course);
    ModemTransmitStart();
    while (Modem_Is_Transmitting()) {
        PTT_Keepalive();
        HAL_Delay(50);
    }
    ModemTransmitStop();
    PTT_Off();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    last_tx     = HAL_GetTick();
    last_course = gps_data.course;
}

// ── Main loop ─────────────────────────────────────────────────────────────────

void Tracker_Run(void) {
    if (!gps_data.is_valid) return;

    uint32_t now = HAL_GetTick();

    if (device_config.smart_beacon) {
        // SmartBeaconing mode
        uint32_t interval = SmartBeacon_Interval();
        uint8_t  time_due = (now - last_tx) >= interval;
        uint8_t  turn_due = SmartBeacon_TurnDetected() &&
                            ((now - last_tx) >= 5000UL); // min 5s between turn beacons

        if (time_due || turn_due) {
            Transmit_Packet();
        }
    } else {
        // Fixed interval mode
        if ((now - last_tx) >= ((uint32_t)device_config.interval * 60000UL)) {
            Transmit_Packet();
        }
    }
}
