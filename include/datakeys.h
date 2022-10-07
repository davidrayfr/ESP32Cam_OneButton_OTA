// copy this file to wifikeys.h and edit
const char *ssid =     "MaisonRay300";         // Put your SSID here
const char *password = "CamilleEmilie";     // Put your PASSWORD here

//Initial Valeur stored in EEPROM
const EEPROM_Data (INITIAL_VALUE){
                    STRUCT_MAGIC,
                    "STA",
                    "MaisonRay300",
                    "CamilleEmilie",
                    "123456",
                    "Esp32Cam",
                    true,
                    false,
                    554
                };