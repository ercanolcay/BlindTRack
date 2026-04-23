/*
  ******************************************************************************
  * @file           : config.h
  * @brief          : Ercan OLCAY # BlueTRace APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_
#include <stdint.h>

typedef struct {
    uint32_t magic;
    char callsign[10];
    uint8_t ssid;
    uint8_t interval;
    char table_id;
    char symbol;
    char comment[50];
} TrackerConfig_t;


extern TrackerConfig_t device_config;

// Function Prototypes
void Load_Config_From_Flash(void);
void Save_Config_To_Flash(void);
void Parse_Config(char* payload);

#endif /* INC_CONFIG_H_ */
