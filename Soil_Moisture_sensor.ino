
/*********************************************************************

 Author: Alyssa Li

*********************************************************************/

/*  This script displays a certain color on the Neopixel strip depending on the moisture of the soil.
 */

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6
#define NUMPIXELS 11
 

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


int rainPin = A0;
int thresholdValue = 800;

void setup(){
  pinMode(rainPin, INPUT);
  
  strip.begin();
  strip.show();
  
  Serial.begin(9600); 
}

void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(rainPin);
  Serial.print(sensorValue);
  if(sensorValue > 970){
    Serial.println(" Air");
    colorWipe(strip.Color(0, 0, 0), 50); // none
    strip.show();
  }

    else if(sensorValue < 970 && sensorValue > thresholdValue) {
    Serial.println("Dry - Time to water your plant");
    colorWipe(strip.Color(255, 0, 0), 50); // red
    strip.show();
  }
    else {
    Serial.println(" wet - Time to water your plant");
    colorWipe(strip.Color(0, 255, 0), 50); // green
    strip.show();
  }
  delay(500);
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}
