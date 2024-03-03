#include <SPI.h>
#include <Wire.h>
#include <MQ135.h>
#include  <DHT20.h>

#include <UTFT.h>
#include <Touch.h>

#define PIN_MQ135 2


// Declare which fonts we will be using
extern uint8_t SmallFont[];

// Remember to change the model parameter to suit your display module!
UTFT myGLCD(ILI9325D_16,PD11,PD5,PD7,PC1);
Touch myTouch(PA6, PB6, PA5, PB7, PA7);

DHT20 DHT(&Wire);

float h = 25.0;
float t = 21.0;

MQ135 mq135_sensor(PIN_MQ135);

int gasLevel  = 0;         //int variable for gas level
String quality =""; 

// PPD42 Pin Configuration
const int pinLED = 13;    // LED pin on Arduino
const int pinPM = 2;      // PPD42 signal pin connected to Arduino digital pin 2

volatile unsigned long lowpulseoccupancy = 0;
volatile unsigned long millisbegin;

float concentration = 0;

void startDHTSensor()
{

  DHT.read();
  h = DHT.getHumidity();
  t = DHT.getTemperature();
  
  Serial.print(h, 1);
  Serial.print(", ");
  Serial.println(t, 1);
        
  if (isnan(h) || isnan(t)) {
  Serial.println("Failed  to read from DHT sensor!");
    return;
  }
 
  myGLCD.print("Temp  :", LEFT, 50);
//  myGLCD.print(t,CENTER, 50);
  myGLCD.print("C", RIGHT, 50);
  myGLCD.print("RH    :", LEFT, 100);
//  myGLCD.print(h,CENTER, 100);
  myGLCD.print("%d %", RIGHT, 100);
}

void startMQSensor()
{

  float rzero = mq135_sensor.getRZero();
  float correctedRZero = mq135_sensor.getCorrectedRZero(t, h);
  float resistance = mq135_sensor.getResistance();
  float ppm = mq135_sensor.getPPM();
  float correctedPPM = mq135_sensor.getCorrectedPPM(t, h);

  gasLevel = ppm;

  if(gasLevel < 50){
    quality = "  GOOD!";
  }else if (gasLevel > 51 && gasLevel < 100){
    quality =  "  Moderate!";
  }
  else if (gasLevel > 101 && gasLevel < 150){
    quality =  "  Unhealthy for Sensitive Groups!";
  }
  else if (gasLevel > 151 && gasLevel < 200){
    quality  = "  Unhealthy!";
  }
    else if (gasLevel > 201 && gasLevel < 300){
    quality  = "  Very Unhealthy!";
  }
    else{
    quality = "  Hazardous!";   
  }

  Serial.print("MQ135 RZero: ");
  Serial.print(rzero);
  Serial.print("\t Corrected RZero: ");
  Serial.print(correctedRZero);
  Serial.print("\t Resistance: ");
  Serial.print(resistance);
  Serial.print("\t PPM: ");
  Serial.print(ppm);
  Serial.print("\t Corrected PPM: ");
  Serial.print(correctedPPM);
  Serial.println("ppm");
  
  myGLCD.print("Air Quality:", LEFT, 150);
  myGLCD.print(quality, CENTER, 150);  
}

void setup() {

  randomSeed(analogRead(0));

  Serial.begin(9600);

  pinMode(pinLED, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(pinPM), pin_ISR, CHANGE);
  millisbegin = millis();
  
// Setup the LCD
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);

  myTouch.init(320,200,3900,3900);

  Wire.begin();
  DHT.begin();  //  ESP32 default pins 21 22

  Serial.begin(115200);
  Serial.print("Humidity, Temperature");
  
}

void loop() {
  
// Clear the screen and draw the frame
  myGLCD.clrScr();

  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 319, 13);
  myGLCD.setColor(64, 64, 64);
  myGLCD.fillRect(0, 226, 319, 239);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("* Air Quality system *", CENTER, 1);
  myGLCD.setBackColor(64, 64, 64);
  myGLCD.setColor(255,255,0);
  myGLCD.print("mde Fra-UAS  Pollin Board", CENTER, 227);

  startDHTSensor();
  startMQSensor();

  delay(10000);  // Sample for 10 seconds
  
  detachInterrupt(digitalPinToInterrupt(pinPM));
  
  // Calculate the concentration in micrograms per cubic meter
  float ratio = (float)lowpulseoccupancy / ((millis() - millisbegin) * 10.0);
  concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
  
  // Print the calculated concentration
  Serial.print("Concentration: ");
  Serial.print(concentration);
  Serial.println(" ug/m3");

  myGLCD.print("Concentration:", LEFT, 200);
//  myGLCD.print(concentration , CENTER, 200); 
  myGLCD.print(" ug/m3", RIGHT, 200);

  // Reset values for the next iteration
  lowpulseoccupancy = 0;
  millisbegin = millis();
  
  attachInterrupt(digitalPinToInterrupt(pinPM), pin_ISR, CHANGE);
  
}

void pin_ISR() {
  lowpulseoccupancy++;
  digitalWrite(pinLED, !digitalRead(pinLED));  // Toggle the LED on each interrupt
}
