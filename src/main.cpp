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

#define TIME_CONFIG_PORTAL 60 // Time of Portal config open (second)

EEPROM_Data memory;

OV2640 cam;

WebServer server(80);

WiFiServer rtspServer(554);

int64_t previousMillis;

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

void handleRoot()
{
String message="<!DOCTYPE html>";
message +="<html lang='f'>";
message +="        <head>";
message +="        <title>CONFIGURATION ESP32-CAM</title>";
message +="        <meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8'/>";
message +="    </head>";
message +="    <body lang='fr'>";
message +="        <h1>Affichage de la configuration</h1>";
message = message + "        <p>WiFi : "+memory.ssid+"</p>";
message = message + "<p>password : "+memory.password+"</p>";
message = message + "        <p>Hostname : "+memory.hostname+"</p>";
message = message + "        <p>http : "+memory.http_enable+"</p>";
message = message + "        <p>rtsp : "+memory.rtsp_enable+"</p>";
message = message + "        <p>rtsp port : "+memory.rtsp_port+"</p>";
message = message + "    </body>";
message = message + "   </html>";
server.send(200,"text/html",message);
}
 
void http_Config_Portal()
{
  server.on("/",handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Serveur web actif!");
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
  Serial.println("mDNS responder started");
  
  confOTA(memory.hostname,memory.ota_password);

// Configuration LED
  ledcSetup(canalPWM, 5000, 12); // canal = 7, frequence = 5000 Hz, resolution = 12 bits
  
  // enable the standard led on pin 13.
  ledcAttachPin(WHITE_LED_PIN, 7); // Signal PWM broche 4, canal 7.
    
  //pinMode(WHITE_LED_PIN, OUTPUT); // sets the digital pin as output
  pinMode(RED_LED_PIN, OUTPUT); // sets the digital pin as output
     
  // Initiate Led
  //digitalWrite(WHITE_LED_PIN, HIGH);
  ledcWrite(canalPWM, 0);   //  LED blanche éteinte (rapport cyclique 0%)
  digitalWrite(RED_LED_PIN, HIGH); // LED Rouge Etainte

// COnfiguration Bouton
    // link the doubleclick function to be called on a doubleclick event.
    button.attachDoubleClick(doubleClick);
    button.attachClick(simpleClick);
    button.setPressTicks(TIME_LONG_CLICK_START); // that is the time when LongPressStart is called
    button.attachLongPressStart(pressStart);
    button.attachLongPressStop(pressStop);

    // initialisation Od timer interrupt
    My_timer=timerBegin(0,80,true);
    timerAttachInterrupt(My_timer,&onTimer,true);
    timerAlarmWrite(My_timer,TIME_BLINK*1000,true);

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
  ArduinoOTA.handle();
  button.tick();
  server.handleClient();
  rtsp_Stream_Server();
}
