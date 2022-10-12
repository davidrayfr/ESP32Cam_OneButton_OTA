// version du programme avec gestion de :
// OTA
// One Button
// EEPROM
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <WebServer.h>
#include "OTA.h"
#include <WebServer.h>
#include <WiFiClient.h>
#include "OV2640.h"
#include "SimStreamer.h"
#include "OV2640Streamer.h"
#include "CRtspSession.h"
#include "eeprom_Sauv.h"
#include "datakeys.h"
#include "OneButton.h"

#define TIME_CONFIG_PORTAL 60000 // Time of Portal config open (in millisecond)

// Data regarding LED
#define RED_LED_PIN 33 // LED rouge: GPIO 33
#define WHITE_LED_PIN 4 // LED blanche: GPIO 4 - ESP32 CAM
#define BUTTON_PIN 12 // Bouton Branché sur GPIO 12
#define canalPWM 7 // un canal PWM disponible
#define MAX_PWM 128 // Intensity Max Led
int ledBright=0;

// Data Regarding Button Management
#define TIME_LONG_CLICK_DETECTION 5000 // Detection Tps Mini long clic in Millisecondes
#define TIME_LONG_CLICK_START 1000 // Detection start Long Click (seconds)
#define TIME_AFTER_LONG_CLICK 2000 // Detection second click after long clic (seconds)
#define TIME_BLINK 100 // Time - Frequency blink for Led in MilliSecond

const int AMPLITUDE_MAX_LED=200; // Amplitude Max LED 
const int COEF=20; // COEF Evolution Pulse 1 Tres lent 20 Rapide

int longClickId = false;
int pulseFlag=false; // pulseFlag=flase -> Blink pulseFlag= True
int whiteLedStatus = false;
unsigned long pressStartTime;
bool http_Config_Portal_activ=false;

// declaration in advance
void http_Config_Portal_Start();
void http_Config_Portal_Closure();

// Setup a new OneButton on pin PIN_INPUT
// The 2. parameter activeLOW is true, because external wiring sets the button to LOW when pressed.
OneButton button(BUTTON_PIN, true,true);
hw_timer_t *My_timer=NULL;

EEPROM_Data memory;

OV2640 cam;

WebServer server(80);

WiFiServer rtspServer(554);

int64_t previousMillis=0;

// Variables for PRGM

void reset_EEPROM()
{
  Serial.println("Reset EEPROM / Restart ESP32");
  delay(1000);
  ESP.restart();
}

void whiteLedPulse()
{
    //digitalWrite(WHITE_LED_PIN, !digitalRead(WHITE_LED_PIN));
  //ledcWrite(canalPWM, 5+((ledBright*10)%250));   //  LED blanche allumée (rapport cyclique 0,1%!)
  int j=ledBright*COEF;
  // Variation Led Blanche
  ledcWrite(canalPWM,((j/AMPLITUDE_MAX_LED)%2)*(AMPLITUDE_MAX_LED-j%AMPLITUDE_MAX_LED)+(((AMPLITUDE_MAX_LED+j)/AMPLITUDE_MAX_LED)%2)*(j%AMPLITUDE_MAX_LED));
  ledBright++;
  }

void whiteLedBlink()
{
    if (ledcRead(canalPWM)==0) {
              ledcWrite(canalPWM, MAX_PWM);   //  LED blanche éteinte (rapport cyclique 0%)
    }
    else {
    ledcWrite(canalPWM, 0);
    }
}

void redLedBlink()
{
    digitalWrite(RED_LED_PIN, !digitalRead(RED_LED_PIN));
}

void IRAM_ATTR checkTicks() {
  // include all buttons here to be checked
  button.tick(); // just call tick() to check the state.
}

void IRAM_ATTR onTimer() {
  // include all buttons here to be checked
  if (pulseFlag)
  {
      whiteLedPulse();
  }
  else
  {
     whiteLedBlink();
  }  
}

void doubleClick()
{
Serial.println("Double Click detected > Clignotement LED BLANCHE");
// Blink launch
pulseFlag=false;
timerAlarmEnable(My_timer);
}

void simpleClick()
{
Serial.println("Simple Click detected");
// Verificaton si clic after long clic

if (longClickId) {
  if (millis()-previousMillis<TIME_AFTER_LONG_CLICK){
    reset_EEPROM();
    }
  else{
    longClickId=false;
    timerAlarmDisable(My_timer);
    ledcWrite(canalPWM, 0);   //  LED blanche éteinte (rapport cyclique 0%)
    }
  }
 else
  {
  if(http_Config_Portal_activ)    
    http_Config_Portal_Start();
    else
    http_Config_Portal_Closure();
  }

}

// Long press detected
// Wait Second short clic to start in the TIME
// LED Blink
void longClick()
{
Serial.println("Long Click");
longClickId=true;
pulseFlag=false;
timerAlarmEnable(My_timer);
//Wait second clic during TIME_AFTER_LONG_CLICK
previousMillis=millis();
http_Config_Portal_activ=false;
}

// this function will be called when the button was held down for 1 second or more.
void pressStart() {
  Serial.println("pressStart()");
  if (timerAlarmEnabled(My_timer)) {
    timerAlarmDisable(My_timer);
    longClickId=false;
    digitalWrite(RED_LED_PIN, HIGH);
    ledcWrite(canalPWM, 0);   //  LED blanche éteinte (rapport cyclique 0%)
  }
  pressStartTime = millis() - TIME_LONG_CLICK_START; // as set in setPressTicks()
} // pressStart()

// this function will be called when the button was released after a long hold.
void pressStop() {
  Serial.print("pressStop(");
  Serial.print(millis() - pressStartTime);
  Serial.println(") detected.");
  if ((millis() - pressStartTime)>TIME_LONG_CLICK_DETECTION)
    {
      Serial.println("long hold detected / More than TIME_LONG_CLICK_START");
      longClick();
    }
  } // pressStop()

// This function start after long press following by short clic

void button_reset()
{

}

void handle_jpg_stream(void)
{
    WiFiClient client = server.client();
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
    server.sendContent(response);

    while (1)
    {
        cam.run();
        if (!client.connected())
            break;
        response = "--frame\r\n";
        response += "Content-Type: image/jpeg\r\n\r\n";
        server.sendContent(response);

        client.write((char *)cam.getfb(), cam.getSize());
        server.sendContent("\r\n");
        if (!client.connected())
            break;
    }
}

void handle_jpg(void)
{
    WiFiClient client = server.client();

    cam.run();
    if (!client.connected())
    {
        return;
    }
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-disposition: inline; filename=capture.jpg\r\n";
    response += "Content-type: image/jpeg\r\n\r\n";
    server.sendContent(response);
    client.write((char *)cam.getfb(), cam.getSize());
}

void handleNotFound()
{
    String message = "Server is running!\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    server.send(200, "text/plain", message);
}

// HTML Launch for Restart
void handleRESTART()
{
  Serial.println("Reset EEPROM / Restart ESP32");
  delay(1000);
  ESP.restart();
}

// HTML Launch for EEPROM Save
void handleSAVE()
{
  Serial.println("Save EEPROM / Restart ESP32");
  delay(1000);
  ESP.restart();
}

void handleRoot()
{
char valeur[20];
itoa((previousMillis+TIME_CONFIG_PORTAL-millis())/1000,valeur,10);
String message="<!DOCTYPE html>";
message +="<html lang='f'>";
message +="<head>\n";
message +="<title>CONFIGURATION ESP32-CAM</title>\n";
message +="<meta http-equiv='refresh' content='3' name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8'/>\n";
message +="</head>\n";
message +="<body lang='fr'>\n";
message +="<h1>Configuration ESP32-CAM</h1>\n";
message +="<h3> Access Point\n";
message = message + "<p>IP   : "+WiFi.localIP().toString()+"</p>";
message = message + "<p>WiFi : "+memory.ssid+"</p>";
message = message + "<p>password : "+memory.password+"</p>\n";
message = message + "<p>Hostname : "+memory.hostname+"</p>\n";
message = message + "<p>http : "+memory.http_enable+"</p>\n";
message = message + "<p>rtsp : "+memory.rtsp_enable+"</p>\n";
message = message + "<p>rtsp port : "+memory.rtsp_port+"</p>\n";
message = message + "<p>time before restart (seconds) : "+valeur+"</p>\n";
message += "echo 'Date et heure actuelle : date (\"d/m/Y H:i\")";
message += "<a class=\"button button-off\" href=\"/save\">SAVE</a>\n";
message += "<a class=\"button button-off\" href=\"/restart\">RESTART</a>\n";
message = message + "</h3></body>\n";
message = message + "</html>\n";
server.send(200,"text/html",message);
}
 
void http_Config_Portal_Start()
{
  previousMillis=millis();
  timerAlarmEnable(My_timer);
  pulseFlag=true;
  server.on("/",handleRoot);
  server.on("/restart",handleRESTART);
  server.on("/save",handleSAVE);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Serveur web actif!");
  previousMillis=millis();
  http_Config_Portal_activ=true;
}

void http_Config_Portal_Closure()
{
  http_Config_Portal_activ=false;
  timerAlarmDisable(My_timer);
  ledcWrite(canalPWM, 0);   //  LED blanche éteinte (rapport cyclique 0%) 
  Serial.println("time ended / http Portal Closure");
  delay(2000);
  server.close();
}

CStreamer *streamer;
CRtspSession *session;
WiFiClient client; // FIXME, support multiple clients

void http_Stream_Server()
{
  server.on("/", HTTP_GET, handle_jpg_stream);
  server.on("/jpg", HTTP_GET, handle_jpg);
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  Serial.println("Start ESP32CAM prgm");
  
  //Camera initialisation
  cam.init(esp32cam_aithinker_config);
  
  // EEPROM Loading
  if (loadEEPROM(memory)) {
    Serial.println("EEPROM Load done");
    }
  else
    {
    Serial.println("EEPROM Empty");
    memory=INITIAL_VALUE;
    }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
    }
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(F("WiFi connected"));
  Serial.print("Adresse MAC :");
  Serial.println(WiFi.macAddress());
  /*use mdns for host name resolution*/
  if (!MDNS.begin(memory.hostname)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
      }
    }
  Serial.print("mDNS responder started with");
  Serial.println(memory.hostname);
  confOTA(memory.hostname,memory.ota_password);

  // Configuration LED
   Serial.println("ledcSetup(canalPWM, 5000, 12)");
  ledcSetup(canalPWM, 5000, 12); // canal = 7, frequence = 5000 Hz, resolution = 12 bits
  
  // enable the standard led on pin 13.
   Serial.println("ledcAttachPin(WHITE_LED_PIN, 7)");
  ledcAttachPin(WHITE_LED_PIN, 7); // Signal PWM broche 4, canal 7.
  ledcWrite(canalPWM, 0);   //  LED blanche éteinte (rapport cyclique 0%) 
  
  //pinMode(WHITE_LED_PIN, OUTPUT); // sets the digital pin as output
   Serial.println("pinMode(RED_LED_PIN, OUTPUT)");
  pinMode(RED_LED_PIN, OUTPUT); // sets the digital pin as output
     
  // Initiate Led
  //digitalWrite(WHITE_LED_PIN, HIGH);
  Serial.println("Allumage LED RED");
  digitalWrite(RED_LED_PIN, HIGH); // LED Rouge Etainte

// COnfiguration Bouton
    // link the doubleclick function to be called on a doubleclick event.
    button.attachDoubleClick(doubleClick);
    button.attachClick(simpleClick);
    button.setPressTicks(TIME_LONG_CLICK_START); // that is the time when LongPressStart is called
    button.attachLongPressStart(pressStart);
    button.attachLongPressStop(pressStop);
    Serial.println("fin configuration Bouton");

    // initialisation Od timer interrupt
    My_timer=timerBegin(0,80,true);
    timerAttachInterrupt(My_timer,&onTimer,true);
    timerAlarmWrite(My_timer,TIME_BLINK*1000,true);
Serial.println("fin configuration Timer");

// Server Start

// Start in http Stream
if (memory.http_enable) http_Stream_Server();

// Start in config Portal

// Start rtspServer
if (memory.rtsp_enable) rtspServer.begin();
}

void rtsp_Stream_Server()
{
// Max Frame
    uint32_t msecPerFrame = 100;
    static uint32_t lastimage = millis();

    // If we have an active client connection, just service that until gone
    // (FIXME - support multiple simultaneous clients)
    if(session) {
        session->handleRequests(0); // we don't use a timeout here,
        // instead we send only if we have new enough frames
        uint32_t now = millis();
        if(now > lastimage + msecPerFrame || now < lastimage) { // handle clock rollover
            session->broadcastCurrentFrame(now);
            lastimage = now;

            // check if we are overrunning our max frame rate
            now = millis();
            if(now > lastimage + msecPerFrame)
                printf("warning exceeding max frame rate of %d ms\n", now - lastimage);
        }

        if(session->m_stopped) {
            delete session;
            delete streamer;
            session = NULL;
            streamer = NULL;
        }
    }
    else {
        client = rtspServer.accept();

        if(client) {
            //streamer = new SimStreamer(&client, true);             // our streamer for UDP/TCP based RTP transport
            streamer = new OV2640Streamer(&client, cam);             // our streamer for UDP/TCP based RTP transport
            session = new CRtspSession(&client, streamer); // our threads RTSP session and state
        }
    }
}

void loop() {
  //ArduinoOTA.handle();
  button.tick();
  server.handleClient();

  // Check if Config Portal open
  if (((previousMillis+TIME_CONFIG_PORTAL)<millis()) and (http_Config_Portal_activ))
    http_Config_Portal_Closure();
  //rtsp_Stream_Server();
}
