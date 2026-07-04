/*
  ******************************************************************************
  * @file           : config.c
  * @brief          : Ercan OLCAY # BlindTRack APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#include "config.h"
#include "main.h"
#include <string.h>
#include <stdlib.h>

// Blue Pill: 64KB Flash, 1 Page = 1024 Byte. Last Page: Page 63
#define FLASH_ADDR_CONFIG 0x0800FC00
#define CONFIG_MAGIC      0x1A2B3C4E  // Bumped magic — struct changed, force re-init

TrackerConfig_t device_config;

void Save_Config_To_Flash(void) {
    device_config.magic = CONFIG_MAGIC;
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError = 0;
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_ADDR_CONFIG;
    EraseInitStruct.NbPages     = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
        HAL_FLASH_Lock();
        return;
    }

    uint32_t *data_ptr = (uint32_t *)&device_config;
    int num_words = (sizeof(TrackerConfig_t) + 3) / 4;
    for (int i = 0; i < num_words; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_ADDR_CONFIG + (i * 4), data_ptr[i]);
    }
    HAL_FLASH_Lock();
}

void Load_Config_From_Flash(void) {
    TrackerConfig_t *flash_config = (TrackerConfig_t *)FLASH_ADDR_CONFIG;
    if (flash_config->magic == CONFIG_MAGIC) {
        memcpy(&device_config, flash_config, sizeof(TrackerConfig_t));
    } else {
        // Factory defaults
        strncpy(device_config.callsign, "N0CALL", sizeof(device_config.callsign) - 1);
        device_config.callsign[sizeof(device_config.callsign) - 1] = '\0';
        device_config.ssid          = 12;
        device_config.interval      = 2;
        device_config.table_id      = '\\';
        device_config.symbol        = ')';
        strncpy(device_config.comment, "BlindTRack APRS Tracker", sizeof(device_config.comment) - 1);
        device_config.comment[sizeof(device_config.comment) - 1] = '\0';
        // SmartBeaconing defaults
        device_config.smart_beacon  = 0;
        device_config.sb_low_speed  = 5;
        device_config.sb_high_speed = 90;
        device_config.sb_slow_rate  = 20;
        device_config.sb_fast_rate  = 30;
        device_config.sb_turn_min   = 30;
        device_config.sb_turn_slope = 255;
        Save_Config_To_Flash();
    }
}

void Parse_Config(char* payload) {
    char* token = strtok(payload, ",");
    if (token) {
        strncpy(device_config.callsign, token, sizeof(device_config.callsign) - 1);
        device_config.callsign[sizeof(device_config.callsign) - 1] = '\0';
    }
    token = strtok(NULL, ",");
    if (token) {
        int v = atoi(token);
        device_config.ssid = (v < 0) ? 0 : (v > 15) ? 15 : (uint8_t)v;
    }
    token = strtok(NULL, ",");
    if (token) {
        int v = atoi(token);
        device_config.interval = (v < 1) ? 1 : (v > 99) ? 99 : (uint8_t)v;
    }
    token = strtok(NULL, ",");
    if (token && strlen(token) >= 2) {
        device_config.table_id = token[0];
        device_config.symbol   = token[1];
    }
    // SmartBeaconing fields
    token = strtok(NULL, ",");
    if (token) device_config.smart_beacon  = atoi(token) ? 1 : 0;
    token = strtok(NULL, ",");
    if (token) { int v = atoi(token); device_config.sb_low_speed  = (v < 0) ? 0 : (v > 200) ? 200 : (uint8_t)v; }
    token = strtok(NULL, ",");
    if (token) { int v = atoi(token); device_config.sb_high_speed = (v < 0) ? 0 : (v > 200) ? 200 : (uint8_t)v; }
    token = strtok(NULL, ",");
    if (token) { int v = atoi(token); device_config.sb_slow_rate  = (v < 1) ? 1 : (v > 99) ? 99 : (uint8_t)v; }
    token = strtok(NULL, ",");
    if (token) { int v = atoi(token); device_config.sb_fast_rate  = (v < 5) ? 5 : (v > 240) ? 240 : (uint8_t)v; }
    token = strtok(NULL, ",");
    if (token) { int v = atoi(token); device_config.sb_turn_min   = (v < 5) ? 5 : (v > 120) ? 120 : (uint8_t)v; }
    token = strtok(NULL, ",");
    if (token) { int v = atoi(token); device_config.sb_turn_slope = (uint8_t)(v & 0xFF); }
    // Comment is always last
    token = strtok(NULL, "\r\n");
    if (token) {
        strncpy(device_config.comment, token, sizeof(device_config.comment) - 1);
        device_config.comment[sizeof(device_config.comment) - 1] = '\0';
    }
    Save_Config_To_Flash();
}
