/*
  ******************************************************************************
  * @file           : modem.c
  * @brief          : Ercan OLCAY # BlueTRace APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#include "modem.h"
#include "ax25.h"
#include "ptt.h"

extern TIM_HandleTypeDef htim4;

// Sine lookup table (256 steps, 0-255 range for 8-bit PWM)
// Generated for: sine_table[i] = round(127.5 + 127.5 * sin(2*pi*i/256))
static const uint8_t sine_table[256] = {
    128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,
    176,179,182,185,188,190,193,196,198,201,203,206,208,211,213,215,
    218,220,222,224,226,228,230,232,234,235,237,238,240,241,243,244,
    245,246,248,249,250,250,251,252,253,253,254,254,254,255,255,255,
    255,255,255,255,254,254,254,253,253,252,251,250,250,249,248,246,
    245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,220,
    218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,
    176,173,170,167,165,162,158,155,152,149,146,143,140,137,134,131,
    128,124,121,118,115,112,109,106,103,100, 97, 93, 90, 88, 85, 82,
     79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40,
     37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11,
     10,  9,  7,  6,  5,  5,  4,  3,  2,  2,  1,  1,  1,  0,  0,  0,
      0,  0,  0,  0,  1,  1,  1,  2,  2,  3,  4,  5,  5,  6,  7,  9,
     10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35,
     37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76,
     79, 82, 85, 88, 90, 93, 97,100,103,106,109,112,115,118,121,124
};

static uint8_t  is_tx = 0;
static uint32_t phase_acc = 0;
static uint16_t sample_count = 0; // Baud rate timing counter
static uint8_t  current_bit = 1;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4 && is_tx) {
    	// 1200 baud timing: read a new bit every 40 samples
        if (sample_count == 0) {
            current_bit = Ax25GetTxBit();

            // CHECK: Is the packet finished?
            if (current_bit == 0xFF || !Ax25IsSending()) {
                is_tx = 0;
                sample_count = 0;
                ModemTransmitStop();    // Stop PWM!
                PTT_Off();              // PTT OFF
                return;
            }
        }

        // 2. Tone Generation (Phase Accumulator)
        // 32-bit phase increment values for 48kHz sampling:
        // For 1200 Hz: (1200 * 2^32) / 48000 = 107374182
        // For 2200 Hz: (2200 * 2^32) / 48000 = 196852667
        uint32_t phase_inc = (current_bit) ? 107374182 : 196852667;

        phase_acc += phase_inc;

        // PWM duty cycle update via sine lookup table
        // phase_acc >> 24 gives 8-bit index (0-255) into the sine table
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, sine_table[phase_acc >> 24]);

        // Counter Check
        sample_count++;
        if (sample_count >= 40) {
            sample_count = 0; // 40 samples completed; a new bit will be requested on the next interrupt
        }
    }
}

void ModemInit(void) {
    is_tx = 0;
    sample_count = 0;
}

void ModemTransmitStart(void) {
    sample_count = 0;
    phase_acc = 0;
    is_tx = 1;
}

void ModemTransmitStop(void) {
    is_tx = 0;
    sample_count = 0;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
}

uint8_t Modem_Is_Transmitting(void) {
    return is_tx;
}
