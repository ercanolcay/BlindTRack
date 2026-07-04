/*
  ******************************************************************************
  * @file           : config.h
  * @brief          : Ercan OLCAY # BlindTRack APRS Tracker
  * 23.04.2026
  ******************************************************************************
  */
#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_
#include <stdint.h>

typedef struct {
    uint32_t magic;
    char     callsign[10];
    uint8_t  ssid;
    uint8_t  interval;       // Fixed interval (minutes), used when SmartBeaconing is off
    char     table_id;
    char     symbol;
    char     comment[50];
    // SmartBeaconing
    uint8_t  smart_beacon;   // 0 = off, 1 = on
    uint8_t  sb_low_speed;   // km/h — below this: use sb_slow_interval
    uint8_t  sb_high_speed;  // km/h — above this: use sb_fast_interval
    uint8_t  sb_slow_rate;   // minutes — interval when slow/stopped
    uint8_t  sb_fast_rate;   // seconds — interval when fast
    uint8_t  sb_turn_min;    // degrees — minimum turn angle to trigger beacon
    uint8_t  sb_turn_slope;  // turn threshold slope factor (default 255)
} TrackerConfig_t;

extern TrackerConfig_t device_config;

// Function Prototypes
void Load_Config_From_Flash(void);
void Save_Config_To_Flash(void);
void Parse_Config(char* payload);

#endif /* INC_CONFIG_H_ */
