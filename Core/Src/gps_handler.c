/*
  ******************************************************************************
  * @file           : gps_handler.c
  * @brief          : Ercan OLCAY # BlindTRack APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#include "gps_handler.h"
#include <string.h>
#include <stdlib.h>

GPS_Data_t gps_data;

static char rx_buf[128];
static uint8_t buf_idx = 0;

void GPS_Process(uint8_t data) {

    if (data == '\n' || data == '\r') {

        if (buf_idx > 20) {
            rx_buf[buf_idx] = '\0';

            char *p = strstr(rx_buf, "RMC,");

            if (p != NULL) {

                char *parts[15];
                int count = 0;

                parts[0] = p;


                for (int i = 0; p[i] != '\0' && count < 14; i++) {
                    if (p[i] == ',') {
                        p[i] = '\0';
                        parts[++count] = &p[i+1];
                    }
                }


                if (count >= 9) {

                    gps_data.is_valid = (parts[2][0] == 'A');

                    if (gps_data.is_valid) {

                        // ---------------- LAT (DDMM.MMMM)
                        gps_data.lat = atof(parts[3]);
                        if (parts[4][0] == 'S')
                            gps_data.lat *= -1;

                        // ---------------- LON (DDDMM.MMMM)
                        gps_data.lon = atof(parts[5]);
                        if (parts[6][0] == 'W')
                            gps_data.lon *= -1;

                        // ---------------- SPEED (KNOT)
                        gps_data.speed = atof(parts[7]);

                        // ---------------- COURSE
                        gps_data.course = atof(parts[8]);
                    }
                }
            }
        }

        // Clear the buffer when the end of the line is reached or after processing
        buf_idx = 0;
    }
    else {
        if (buf_idx < sizeof(rx_buf) - 1) {
            rx_buf[buf_idx++] = (char)data;
        } else {
            buf_idx = 0;
        }
    }
}
