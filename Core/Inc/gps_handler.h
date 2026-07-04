/*
  ******************************************************************************
  * @file           : gps_handler.h
  * @brief          : Ercan OLCAY # BlindTRack APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#ifndef __GPS_HANDLER_H
#define __GPS_HANDLER_H
#include "main.h"

typedef struct {
    float lat;
    float lon;
    float speed;
    float course;
    uint8_t is_valid;
} GPS_Data_t;

extern GPS_Data_t gps_data;
void GPS_Process(uint8_t data);
#endif
