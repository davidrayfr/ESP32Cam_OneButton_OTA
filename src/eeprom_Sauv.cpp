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

// Value initialisation for Strip
// Magic indicate if value already saved in EEPROM

void affichageMemory(EEPROM_Data memory)
{
  Serial.println("Affichage Memory");
  Serial.println(memory.magic);
  Serial.println(memory.ssid);
  Serial.println(memory.password);
  Serial.println(memory.hostname);
  Serial.println(memory.http_enable);
  Serial.println(memory.rtsp_enable);
  Serial.println(memory.port_rtsp);
}

void sauvegardeEEPROM(EEPROM_Data memory) {
  // Met à jour le nombre magic et le numéro de version avant l'écriture
  memory.magic = STRUCT_MAGIC;
  Serial.println("sauvegarde Memory > EEPROM");
  affichageMemory(memory);
  EEPROM.put(EEPROM_MEMORY_START, memory);
  EEPROM.commit();
}

void chargeEEPROM(EEPROM_Data memory) {
  EEPROM_Data EEPROM_memory;
// Lit la mémoire EEPROM
 // EEPROM.begin(1024);
  Serial.println("Chargement Memory LED depuis EEPROM");
  EEPROM.get(EEPROM_MEMORY_START, EEPROM_memory);
 // EEPROM.end();
// Détection d'une mémoire non initialisée
  if (EEPROM_memory.magic != STRUCT_MAGIC) {
      // Engegistrement des valeurs par défaut
      Serial.print("EEPROM_memory.magic");
      Serial.println(EEPROM_memory.magic);
      Serial.println("Magic NOK / EEPROM non initialisé");
      sauvegardeEEPROM(memory);
      }
      else
      memory = EEPROM_memory;
  }
