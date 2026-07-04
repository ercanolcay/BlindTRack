/*
  ******************************************************************************
  * @file           : ax25.h
  * @brief          : Ercan OLCAY # BlindTRack APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#ifndef __AX25_H
#define __AX25_H
#include "main.h"

void Ax25Init(void);
void Ax25WriteTrackerFrame(float lat, float lon, float speed, float course);
uint8_t Ax25GetTxBit(void);
uint8_t Ax25IsSending(void);
#endif
