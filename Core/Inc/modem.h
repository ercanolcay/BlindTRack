/*
  ******************************************************************************
  * @file           : modem.h
  * @brief          : Ercan OLCAY # BlindTRack APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#ifndef __MODEM_H
#define __MODEM_H
#include "main.h"

void ModemInit(void);
void ModemTransmitStart(void);
void ModemTransmitStop(void);
uint8_t Modem_Is_Transmitting(void);
#endif
