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
void whiteLedChange();
void redLedChange();
void whiteLedEvolution();

void IRAM_ATTR checkTicks() {
  // include all buttons here to be checked
  button.tick(); // just call tick() to check the state.
}

void IRAM_ATTR onTimer() {
  // include all buttons here to be checked
  if (longClickId)
  {
      whiteLedChange();
      redLedChange();
  }
  else
  {
     whiteLedEvolution();
  }  
}

void whiteLedEvolution()
{
    //digitalWrite(WHITE_LED_PIN, !digitalRead(WHITE_LED_PIN));
  //ledcWrite(canalPWM, 5+((ledBright*10)%250));   //  LED blanche allumée (rapport cyclique 0,1%!)
  int j=ledBright*COEF;
  // Variation Led Blanche
  ledcWrite(canalPWM,((j/AMPLITUDE_MAX_LED)%2)*(AMPLITUDE_MAX_LED-j%AMPLITUDE_MAX_LED)+(((AMPLITUDE_MAX_LED+j)/AMPLITUDE_MAX_LED)%2)*(j%AMPLITUDE_MAX_LED));
  ledBright++;
  }

void whiteLedChange()
{
    if (ledcRead(canalPWM)==0) {
              ledcWrite(canalPWM, MAX_PWM);   //  LED blanche éteinte (rapport cyclique 0%)
    }
    else {
    ledcWrite(canalPWM, 0);
    }
}

void redLedChange()
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
if (timerAlarmEnabled(My_timer)) {
  timerAlarmDisable(My_timer);
  longClickId=false;
  digitalWrite(RED_LED_PIN, HIGH);
  ledcWrite(canalPWM, 0);   //  LED blanche éteinte (rapport cyclique 0%)
  }
  else
  {
  whiteLedChange();
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

void setup()
{
    Serial.begin(115200);
    Serial.println("One Button Example with ESP-32 Cam");
 // PWM pour LED Blanche
    ledcSetup(canalPWM, 5000, 12); // canal = 7, frequence = 5000 Hz, resolution = 12 bits
  
    // enable the standard led on pin 13.
    ledcAttachPin(WHITE_LED_PIN, 7); // Signal PWM broche 4, canal 7.
    
    //pinMode(WHITE_LED_PIN, OUTPUT); // sets the digital pin as output
    pinMode(RED_LED_PIN, OUTPUT); // sets the digital pin as output
     
    // Initiate Led
    //digitalWrite(WHITE_LED_PIN, HIGH);
    ledcWrite(canalPWM, 0);   //  LED blanche éteinte (rapport cyclique 0%)

    digitalWrite(RED_LED_PIN, HIGH);

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
}
