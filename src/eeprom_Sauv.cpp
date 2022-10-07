// Gestion eeprom
#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_MEMORY_START 512
#define EEPROM_MEMORY_SIZE 1024

static const unsigned long STRUCT_MAGIC = 123456789;

/*
WIFI_OFF     WIFI_MODE_NULL
WIFI_STA     WIFI_MODE_STA
WIFI_AP      WIFI_MODE_AP
WIFI_AP_STA  WIFI_MODE_APSTA */

struct EEPROM_Data {
  unsigned long magic;
  char WiFiMode[4];
  char ssid[32];
  char password[16];
  char ota_password[16];
  char hostname[64];
  char http_enable;
  char rtsp_enable;
  unsigned short rtsp_port;
  };

// Value initialisation for Strip
// Magic indicate if value already saved in EEPROM

void showMemory(EEPROM_Data memory)
{
  Serial.println("Affichage Memory");
  Serial.println(memory.WiFiMode);
  Serial.println(memory.magic);
  Serial.println(memory.ssid);
  Serial.println(memory.password);
  Serial.println(memory.hostname);
  Serial.println(memory.http_enable);
  Serial.println(memory.rtsp_enable);
  Serial.println(memory.rtsp_port);
}

void saveEEPROM(EEPROM_Data memory) {
  // Met à jour le nombre magic et le numéro de version avant l'écriture
  memory.magic = STRUCT_MAGIC;
  Serial.println("sauvegarde Memory > EEPROM");
  showMemory(memory);
  EEPROM.put(EEPROM_MEMORY_START, memory);
  EEPROM.commit();
}

bool loadEEPROM(EEPROM_Data memory) {
  EEPROM_Data EEPROM_memory;
// Lit la mémoire EEPROM
 // EEPROM.begin(1024);
  Serial.println("Load Data from EEPROM");
  EEPROM.get(EEPROM_MEMORY_START, EEPROM_memory);
 // EEPROM.end();
// Détection d'une mémoire non initialisée
  if (EEPROM_memory.magic != STRUCT_MAGIC)
      {
      Serial.println("Magic NOK / EEPROM non initialisé");
      return false;
      }
      else
      {
      Serial.println("Recover EEPROM Data");
      memory = EEPROM_memory;
      return true;
      }
  }