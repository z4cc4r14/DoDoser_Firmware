#ifndef _MYTYPE_
#define _MYTYPE_
#include <EEPROM.h>
#include <Arduino.h>  // for type definitions
#include <stdint.h>
#include <Time.h>
#include <ArduinoJson.h>
//**********************************************************
//
//**********************************************************
#define NUM_OF_PUMP 8
#define NUM_OF_LINE 8
#define NUM_OF_SCHEDULER 150//70

#define SERVER_PORT 60711
#define MAX_DATA 350

DynamicJsonBuffer doseType;
/**
 * @brief Use brief, otherwise the index won't have a brief explanation.
 *
 * Detailed explanation.
 */
typedef struct {
  uint8_t active;/**< flag di attivazione pompa. */
  uint8_t state; /**< flag di stato#b. */
  float counter; /**< contatore#c. */
  uint32_t lastCounterValue;/**< contatore#d. */
  uint8_t pin;
}pump_Type;

typedef struct {
  uint8_t ABILITATION;
  float LEVEL;
  uint8_t ID_PUMP;
  float CAPACITY;
  uint32_t CALIBRATION;
  uint8_t TIMER_ABILITATION;
}line_type;

typedef struct {
  uint8_t DHCP;
  char IP[15];
  char PORT[5];
  char NETMASK[15];
  char GATEWAY[15];
  char SSID[32];
  char PASSWORD[32];
  }network_conf_type;
  
typedef struct {
  uint8_t ID;
  uint8_t HOUR;
  uint8_t MINUTE;
  uint8_t EXEC;
  uint8_t ID_LINE;
  float QUANTITY;
  uint8_t DAYOFWEEK;
}timerDose_type;

timerDose_type dose[NUM_OF_SCHEDULER];
uint32_t totTime;
uint32_t totTimeSum;

#define CMD_DOSE 1
#define CMD_SYNC  3
#define CMD_REFILL 4
#define CMD_SEND_SCHEDULER 5
#define CMD_SET_TIME 6
#define CMD_BREAK_DOSE 7
#define CMD_ERROR_RETURN 8
#define CMD_SCHEDULER 9
#define CMD_STATUS  10
//#define CMD_CONFIG 11
#define CMD_CONFIG_IP 12
#define CMD_CONFIG_CAPACITY 17
#define CMD_CONFIG_ABLE 14
#define CMD_CONFIG_TIME_ABLE  15
#define CMD_CONFIG_CALIB  16

#define DEBUG 0

//ERROR TYPE
#define ERROR_LINE_NOT_ABLE 0
#define ERROR_NO_PUMP_ASSOCIATED 1
#define ERROR_PUMP_ALREADY_STOPED 2 
#define ERROR_LINE_NOT_FREE 3
#define ERROR_NO_RTC_COM 4
#define ERROR_NO_CALIB_VALUE 5
#define LED 13

//EEPROM MAP, 512Byte

#define LEVEL_SOLUTION_1_BASE  0
#define LEVEL_SOLUTION_2_BASE  4
#define LEVEL_SOLUTION_3_BASE  8
#define LEVEL_SOLUTION_4_BASE  12

#define SLAVE_A_ADDRESS 4

#define EEPROM_LINE_DATA_ADDRESS_L  0
#define EEPROM_LINE_DATA_ADDRESS_H 255
#define EEPROM_SCHEDULER_DATA_ADDRESS_L 255 
#define EEPROM_SCHEDULER_DATA_ADDRESS_H 1755//1055 
#endif



