/*
  ******************************************************************************
  * @file           : ptt.c
  * @brief          : Ercan OLCAY # BlueTRace APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#include "ptt.h"
#include "main.h"

extern UART_HandleTypeDef huart2; // AnyTone on PA2 (UART2)

uint8_t PTT_ON_CMD[]  = {0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06};
uint8_t PTT_OFF_CMD[] = {0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06};
uint8_t PTT_KEEPALIVE_CMD[] = {0x06};

void PTT_On(void) {
    // 1. AnyTone TX Command
    HAL_UART_Transmit(&huart2, PTT_ON_CMD, 8, 100);

    // 2. Green Led ON
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

void PTT_Off(void) {
    // 1. AnyTone release the ptt
    HAL_UART_Transmit(&huart2, PTT_OFF_CMD, 8, 100);

    // 2. Green Led OFF
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
}

void PTT_Keepalive(void) {
    HAL_UART_Transmit(&huart2, PTT_KEEPALIVE_CMD, 1, 10);
}
