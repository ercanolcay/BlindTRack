/*
  ******************************************************************************
  * @file           : config.c
  * @brief          : Ercan OLCAY # BlueTRace APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */

#include "config.h"
#include "main.h"
#include <string.h>
#include <stdlib.h>

// Blue Pill: 64KB Flash, 1 Page= 1024 Byte. Last Page: Page 63
#define FLASH_ADDR_CONFIG 0x0800FC00
#define CONFIG_MAGIC      0x1A2B3C4D // BlueTRace Magic Signature

// Global variables
TrackerConfig_t device_config;

void Save_Config_To_Flash(void) {
    device_config.magic = CONFIG_MAGIC; // Sign it.

    HAL_FLASH_Unlock(); // Unlock Flash

    // Delete Last Page
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError = 0;

    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_ADDR_CONFIG;
    EraseInitStruct.NbPages     = 1;

    // If a deletion error occurs, cancel the operation and lock it
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
        HAL_FLASH_Lock();
        return;
    }

    // Write the struct to Flash in 32-bit (word) packets
    uint32_t *data_ptr = (uint32_t *)&device_config;
    int num_words = (sizeof(TrackerConfig_t) + 3) / 4; // Pad to multiples of 4 bytes

    for (int i = 0; i < num_words; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_ADDR_CONFIG + (i * 4), data_ptr[i]);
    }

    HAL_FLASH_Lock(); // Flash yazma bitti, kilitle
}

void Load_Config_From_Flash(void) {
    // Check the address in flash with pointer
    TrackerConfig_t *flash_config = (TrackerConfig_t *)FLASH_ADDR_CONFIG;

    if (flash_config->magic == CONFIG_MAGIC) {
        // Signature is ok! Settings were previously saved; copy them to RAM (device_config)
        memcpy(&device_config, flash_config, sizeof(TrackerConfig_t));
    } else {
    	// Flash is empty (or the device is running for the first time). Load factory defaults.
    	strncpy(device_config.callsign, "N0CALL", sizeof(device_config.callsign) - 1);
    	device_config.callsign[sizeof(device_config.callsign) - 1] = '\0';
    	device_config.ssid = 12;
    	device_config.interval = 2;
    	device_config.table_id = '\\'; // Default: Secondary Table
    	device_config.symbol = ')';
    	strncpy(device_config.comment, "BlueTRace Aprs Tracker", sizeof(device_config.comment) - 1);
    	device_config.comment[sizeof(device_config.comment) - 1] = '\0';

        Save_Config_To_Flash(); // Upload defaults to flash
    }
}

void Parse_Config(char* payload) {
    char* token = strtok(payload, ",");
    if(token) {
        strncpy(device_config.callsign, token, sizeof(device_config.callsign) - 1);
        device_config.callsign[sizeof(device_config.callsign) - 1] = '\0';
    }

    token = strtok(NULL, ",");
    if(token) {
        int ssid = atoi(token);
        device_config.ssid = (ssid < 0) ? 0 : (ssid > 15) ? 15 : (uint8_t)ssid;
    }

    token = strtok(NULL, ",");
    if(token) {
        int interval = atoi(token);
        device_config.interval = (interval < 1) ? 1 : (interval > 99) ? 99 : (uint8_t)interval;
    }

    token = strtok(NULL, ",");
    if(token && strlen(token) >= 2) {
        device_config.table_id = token[0];
        device_config.symbol = token[1];
    }

    // The comment may contain commas or spaces, so read until "\r" or "\n" is encountered
    token = strtok(NULL, "\r\n");
    if(token) {
        strncpy(device_config.comment, token, sizeof(device_config.comment) - 1);
        device_config.comment[sizeof(device_config.comment) - 1] = '\0';
    }

    // New settings have been successfully applied to RAM; save to Flash to make them persistent!
    Save_Config_To_Flash();
}
