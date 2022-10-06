// Gestion eeprom
#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_MEMORY_START 512
#define EEPROM_MEMORY_SIZE 1024
static const unsigned long STRUCT_MAGIC = 123456789;

struct EEPROM_Data {
  unsigned long magic;
  char ssid[32];
  char password[16];
  char ota_password[16];
  char hostname[64];
  char http_enable;
  char rtsp_enable;
  unsigned short port_rtsp;
  };

void affichageMemory(EEPROM_Data memory);

void sauvegardeEEPROM(EEPROM_Data memory);

void chargeEEPROM(EEPROM_Data memory);