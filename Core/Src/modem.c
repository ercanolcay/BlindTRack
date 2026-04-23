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

static uint8_t  is_tx = 0;
static uint32_t phase_acc = 0;
static uint16_t sample_count = 0; // Baud rate timing counter
static uint8_t  current_bit = 1;  //

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

        // PWM duty cycle update (if you don't have a sine table, generates a sawtooth,
        // but the Radio input stage will filter it)
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, (phase_acc >> 24));

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
