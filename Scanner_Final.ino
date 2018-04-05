/*********************************************************************
 Author: Alyssa Li
*********************************************************************/

/*  This example scans for advertising devices (peripherals) in range,
 *  looking for a specific UUID in the advertising packet. When this
 *  UUID is found, the neopixel ring would change color to the relevant module.
 *  
 *  This example is intended to be used with multiple peripherals
 *  devices running the *_peripheral.ino version of this application.
 *  
 *  VERBOSE_OUTPUT
 *  --------------
 *  Verbose advertising packet analysis can be enabled for
 *  advertising packet contents to help debug issues with the 
 *  advertising payloads, with fully parsed data displayed in the 
 *  Serial Monitor.
 
 */

#include <string.h>
#include <bluefruit.h>
#include <SPI.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 7
#define NUMPIXELS 16

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);

#define VERBOSE_OUTPUT (0)    // Set this to 1 for verbose adv packet output to the serial monitor

const int ledPin = 7;       // the pin that the LED is attached to
int alarmState = 0;         // 0: not here; 1: here

// Custom UUID used to differentiate this device.
// Use any online UUID generator to generate a valid UUID.
// Note that the byte order is reversed ... CUSTOM_UUID
// below corresponds to the  follow value:
// df67ff1a-718f-11e7-8cf7-a6006ad3dba0
const uint8_t CUSTOM_UUID[] =
{
    0xA0, 0xDB, 0xD3, 0x6A, 0x00, 0xA6, 0xF7, 0x8C,
    0xE7, 0x11, 0x8F, 0x71, 0x1A, 0xFF, 0x67, 0xDF
};

BLEUuid uuid = BLEUuid(CUSTOM_UUID);

void setup()
{
  //pinMode(ledPin, OUTPUT);
  strip.begin();
  strip.show();
  Serial.begin(115200);
  Serial.println("Critical Making Provo 2 2018");
  Serial.println("-------------------------------------\n");

  /* Enable both peripheral and central modes */
  err_t err = Bluefruit.begin(true, true);
  if (err)
  {
    Serial.print("Unable to init Bluefruit (ERROR CODE: ");
    Serial.print(err);
    Serial.println(")");
    while(1)
    {
      digitalToggle(LED_RED);
      delay(100);
    }
  }
  else
  {
    Serial.println("Bluefruit initialized (central mode)");
  }
  
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(0);

  /* Set the device name */
  Bluefruit.setName("Central");

  // off Blue LED for lowest power consumption
  Bluefruit.autoConnLed(false);

  /* Set the LED interval for blinky pattern on BLUE LED */
  Bluefruit.setConnLedInterval(250);

  /* Start Central Scanning
   * - Enable auto scan if disconnected
   * - Filter out packet with a min rssi
   * - Interval = 100 ms, window = 50 ms
   * - Use active scan (used to retrieve the optional scan response adv packet)
   * - Start(0) = will scan forever since no timeout is given
   */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.filterRssi(-70);            // Only invoke callback for devices with RSSI >= -80 dBm
  Bluefruit.Scanner.filterUuid(uuid);           // Only invoke callback if the target UUID was found
  //Bluefruit.Scanner.filterMSD(0xFFFF);          // Only invoke callback when MSD is present with the specified Company ID
  Bluefruit.Scanner.setInterval(160, 80);       // in units of 0.625 ms
  Bluefruit.Scanner.useActiveScan(true);        // Request scan response data
  Bluefruit.Scanner.start(0);                   // 0 = Don't stop scanning after n seconds
  Serial.println("Scanning ...");
}

/* This callback handler is fired every time a valid advertising packet is detected */
void scan_callback(ble_gap_evt_adv_report_t* report)
{
  Serial.println("Scan_callback invoked");
  alarmState = 1;

//  Serial.print("alarmState: ");
//  Serial.println(alarmState);
//  Serial.println();
  

/* Fully parse and display the advertising packet to the Serial Monitor
 * if verbose/debug output is requested */
#if VERBOSE_OUTPUT
  uint8_t len = 0;
  uint8_t buffer[32];
  memset(buffer, 0, sizeof(buffer));

  /* Display the timestamp and device address */
  if (report->scan_rsp)
  {
    Serial.printf("[SR%10d] Packet received from ", millis());
  }
  else
  {
    Serial.printf("[ADV%9d] Packet received from ", millis());
  }
  // MAC is in little endian --> print reverse
  Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
  Serial.println("");
  
  /* Raw buffer contents */
  Serial.printf("%14s %d bytes\n", "PAYLOAD", report->dlen);
  if (report->dlen)
  {
    Serial.printf("%15s", " ");
    Serial.printBuffer(report->data, report->dlen, '-');
    Serial.println();
  }

  /* RSSI value */
  Serial.printf("%14s %d dBm\n", "RSSI", report->rssi);

  /* Adv Type */
  Serial.printf("%14s ", "ADV TYPE");
  switch (report->type)
  {
    case BLE_GAP_ADV_TYPE_ADV_IND:
      Serial.printf("Connectable undirected\n");
      break;
    case BLE_GAP_ADV_TYPE_ADV_DIRECT_IND:
      Serial.printf("Connectable directed\n");
      break;
    case BLE_GAP_ADV_TYPE_ADV_SCAN_IND:
      Serial.printf("Scannable undirected\n");
      break;
    case BLE_GAP_ADV_TYPE_ADV_NONCONN_IND:
      Serial.printf("Non-connectable undirected\n");
      break;
  }

  /* Shortened Local Name */
  if(Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, buffer, sizeof(buffer)))
  {
    Serial.printf("%14s %s\n", "SHORT NAME", buffer);
    memset(buffer, 0, sizeof(buffer));
  }

  /* Complete Local Name */
  if(Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, buffer, sizeof(buffer)))
  {
    Serial.printf("%14s %s\n", "COMPLETE NAME", buffer);
    memset(buffer, 0, sizeof(buffer));
  }

  /* TX Power Level */
  if (Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_TX_POWER_LEVEL, buffer, sizeof(buffer)))
  {
    Serial.printf("%14s %i\n", "TX PWR LEVEL", buffer[0]);
    memset(buffer, 0, sizeof(buffer));
  }

  /* Check for UUID16 Complete List */
  len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE, buffer, sizeof(buffer));
  if ( len )
  {
    printUuid16List(buffer, len);
  }

  /* Check for UUID16 More Available List */
  len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE, buffer, sizeof(buffer));
  if ( len )
  {
    printUuid16List(buffer, len);
  }

  /* Check for UUID128 Complete List */
  len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE, buffer, sizeof(buffer));
  if ( len )
  {
    printUuid128List(buffer, len);
  }

  /* Check for UUID128 More Available List */
  len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE, buffer, sizeof(buffer));
  if ( len )
  {
    printUuid128List(buffer, len);
  }  

  /* Check for BLE UART UUID */
  if ( Bluefruit.Scanner.checkReportForUuid(report, BLEUART_UUID_SERVICE) )
  {
    Serial.printf("%14s %s\n", "BLE UART", "UUID Found!");
  }

  /* Check for DIS UUID */
  if ( Bluefruit.Scanner.checkReportForUuid(report, UUID16_SVC_DEVICE_INFORMATION) )
  {
    Serial.printf("%14s %s\n", "DIS", "UUID Found!");
  }

  /* Check for Manufacturer Specific Data */
  len = Bluefruit.Scanner.parseReportByType(report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, buffer, sizeof(buffer));
  if (len)
  {
    Serial.printf("%14s ", "MAN SPEC DATA");
    Serial.printBuffer(buffer, len, '-');
    Serial.println();
    memset(buffer, 0, sizeof(buffer));
  }

  Serial.println();
#endif
}


/* Prints a UUID16 list to the Serial Monitor */
void printUuid16List(uint8_t* buffer, uint8_t len)
{
  Serial.printf("%14s %s", "16-Bit UUID");
  for(int i=0; i<len; i+=2)
  {
    uint16_t uuid16;
    memcpy(&uuid16, buffer+i, 2);
    Serial.printf("%04X ", uuid16);
  }
  Serial.println();
}

/* Prints a UUID128 list to the Serial Monitor */
void printUuid128List(uint8_t* buffer, uint8_t len)
{
  (void) len;
  Serial.printf("%14s %s", "128-Bit UUID");

  // Print reversed order
  for(int i=0; i<16; i++)
  {
    const char* fm = (i==4 || i==6 || i==8 || i==10) ? "-%02X" : "%02X";
    Serial.printf(fm, buffer[15-i]);
  }

  Serial.println();  
}


void loop() 
{
      
      if(alarmState == 1) // received OK, turn on LEDs
      { 
          //digitalWrite(ledPin, HIGH);
          colorWipe(strip.Color(255, 255, 0), 50); // yellow
          strip.show();
          Serial.println("LEDs are on");
          delay(3000);
          alarmState=0;
       }

      else
      {
        //digitalWrite(ledPin, LOW);
        colorWipe(strip.Color(0, 0, 0), 50); // none
        //strip.begin();
        strip.show();
        Serial.println("LEDs are off");
      }

//      Serial.print("New alarmState: ");
//      Serial.println(alarmState);
//      Serial.println();
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}
