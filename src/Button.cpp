// Bouton sur GPIO 12
// Allumage diode blanche
// Check 1 clic -> start web - configuration portal - pulsation white led
// second clic - stop portal

// Check 2 clic -> Pulsation Led Blanche
// Check Long Clic -> white led clignotement - wait single clic during 5 seconds
// Lorsque que l'on appui sur le bouton > annule les clignotements et eteint la lampe
// Ajout Variation Led

#include "OneButton.h"

#define RED_LED_PIN 33 // LED rouge: GPIO 33
#define WHITE_LED_PIN 4 // LED blanche: GPIO 4 - ESP32 CAM
#define BUTTON_PIN 12 // Bouton Branché sur GPIO 12
#define TIME_LONG_CLICK_DETECTION 5000 // Detection Tps Mini long clic in Millisecondes
#define TIME_LONG_CLICK_START 1000 // Detection start Long Click
#define TIME_AFTER_LONG_CLICK 2000 // Detection second click after long clic
#define TIME_BLINK 100 // Time - Frequency blink for Led in MilliSecond
#define canalPWM 7 // un canal PWM disponible
#define MAX_PWM 128 // Intensity Max Led

const int AMPLITUDE_MAX_LED=255; // Amplitude Max LED 
const int COEF=10;

// Setup a new OneButton on pin PIN_INPUT
// The 2. parameter activeLOW is true, because external wiring sets the button to LOW when pressed.
OneButton button(BUTTON_PIN, true,true);
hw_timer_t *My_timer=NULL;

// current LED state, staring with LOW (0)

int longClickId = false;
int whiteLedStatus = false;
unsigned long pressStartTime;
int ledBright=0;

// Declaration indavance
void whiteLedBlink();
void redLedBlink();
void whiteLedPulse();

void IRAM_ATTR checkTicks() {
  // include all buttons here to be checked
  button.tick(); // just call tick() to check the state.
}

void IRAM_ATTR onTimer() {
  // include all buttons here to be checked
  if (longClickId)
  {
      whiteLedBlink();
  }
  else
  {
     whiteLedPulse();
  }  
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

void doubleClick()
{
Serial.println("Double Click detected > Clignotement LED BLANCHE");
// Blink launch
timerAlarmEnable(My_timer);
}

void simpleClick()
{
Serial.println("Simple Click detected");
// Verificaton si clic after long clic

if (timerAlarmEnabled(My_timer)) {
  timerAlarmDisable(My_timer);
  longClickId=false;
  ledcWrite(canalPWM, 0);   //  LED blanche éteinte (rapport cyclique 0%)
  }
  else
  {
  whiteLedBlink();
  }
}

void longClick()
{
Serial.println("Long Click");
longClickId=true;
timerAlarmEnable(My_timer);
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
      Serial.println("long hold detected");
      longClick();
    }
  } // pressStop()

// This function start after long press following by short clic

void button_reset()
{

}
