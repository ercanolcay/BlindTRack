/*
  ******************************************************************************
  * @file           : tracker.c
  * @brief          : Ercan OLCAY # BlueTRace APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#include "tracker.h"
#include "ptt.h"
#include "gps_handler.h"
#include "ax25.h"
#include "modem.h"
#include "config.h"

uint32_t last_tx = 0xFFF00000;

void Tracker_Run(void) {
    uint32_t now = HAL_GetTick();

    if ((now - last_tx > (device_config.interval * 60000)) && gps_data.is_valid) {
    	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
        PTT_On();
        HAL_Delay(500); // AnyTone wake up

        // Prepare the packet and start transmission
        Ax25WriteTrackerFrame(gps_data.lat, gps_data.lon, gps_data.speed, gps_data.course);
        ModemTransmitStart();

        // Wait until the modem finishes transmission (PA1 must remain high)
        // You can use Modem_Is_Transmitting() or Ax25IsSending()
        while(Modem_Is_Transmitting()) {
            PTT_Keepalive();
            HAL_Delay(50);
        }

        ModemTransmitStop();
        PTT_Off();
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

        last_tx = HAL_GetTick();
    }
}

