/*
 * VFD Clock rev final release 2.0
 * 12 Dec. 2020
 *      Month with name
 *      Date, UTC time, Local time
 *      UTC offset
 * Included a routine to rewrite the VFD to avoid weird things after a while.
 * 
 * Author: YO3HJV, Adrian
 * LICENSE: If you like it and use it, please send me an e-mail
 * at: yo3hjv@gmail.com with a nice picture from your QTH or surroundings.
 * 
 * The time is set with the help of the PC, therefore
 * we will define UTC as a result of the Local time + or -
 * offset.
 * This will affect the date changing; the date will change 
 * according to Local time.
 *   HW: DS1307 on I2C
 *       VFD TTL UART input connect to Tx port defined on mySerial
 *
 * original from https://yo3hjv.blogspot.com/2020/12/shack-dual-time-clock-with-pos-vfd.html
 * small updates by Nicu FLORICA (niq_ro) in switch routine (added break command after each case)
 * v.2 - added DHT22 sensor and remove UT (universal time) info 
 */

#include "DHT.h"  // used Example testing sketch for various DHT humidity/temperature sensors
                  // Written by ladyada, public domain
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>  // https://github.com/PaulStoffregen/DS1307RTC
#include <SoftwareSerial.h>
SoftwareSerial mySerial(3, 2); // RX, TX
#define DHTPIN 7     // what pin we're connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
// NOTE: For working with a faster chip, like an Arduino Due or Teensy, you
// might need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// Example to initialize DHT sensor for Arduino Due:
//DHT dht(DHTPIN, DHTTYPE, 30);

 int utcHour;
 int localHour;
 int localMinutes;
 int localSeconds;
 

 int UTCoffset = -2;          // Here we have the local time offset
 int bright = 3;              // 1 < n < 4
 int rev = 1;                 // 0-normal; 1-reverse
 int blRate = 150;
 byte debuging = 1;           // 1 - serial monitor, 0 - no serial monitor

 int h; // humdity
 float t; // temperature in Celsius
 float f; // temperature in Farehneit
 float hi; // heat index

unsigned long tpdht;
unsigned long tpread = 60000;
unsigned long tpceas;
unsigned long tpreadceas = 1000;

 
void setup() {
  if (debuging == 1)
  {
  Serial.begin(9600);
  while (!Serial) ; // wait for serial
  delay(200);
  }
  Serial.println("RTC clock on VFD CD7220 display v.2");
  Serial.println("-------------------");

    dht.begin(); 
    mySerial.begin(9600);
    delay(10);
   // setDispMode();
         mySerial.write(0x0C);     // Clear ALL
          delay(10);
                                   // Display init
                 mySerial.write(0x1B);
                 mySerial.write(0x40);
         howBright();              // Display brightness
     //    fixedVFD();               // Show legend on VFD
         
readDHT();
tpdht = millis();
hVFD();
tVFD();
}  // end setup

void setDispMode(){
  mySerial.write(0x02);  //  STX
  mySerial.write(0x05);  //  05
  mySerial.write(0x43);  //  C
  mySerial.write(0x31);  //  31h - ESC/POS mode
  mySerial.write(0x03);  //  ETX
}

void loop() {

   if (millis() - tpceas > tpreadceas)
   {
     // I used functions for each element to print
     // It will be easyer to make further mods in the code
     tmElements_t tm;
     RTC.read(tm);  
                // Load global variables from RTC object
    dateVFD();
          localHour = tm.Hour;
         // utcHour = localHour + UTCoffset;
          localMinutes = tm.Minute;
          localSeconds = tm.Second;
  /*        
        utcHour = localHour + UTCoffset;
            if (utcHour < 0)
                 {
                      utcHour += 24;
                 }
              utcVFD();
   */
              loctimeVFD();
    tpceas = millis();
   }
   
if (millis() - tpdht > tpread)
  {
    readDHT();
    tpdht = millis();
    hVFD();
    tVFD();
  }
              
delay(1);              
}  // end main loop



// Print UTC time with the offset in respect to Local time

void utcVFD(){      
                            // set cursor for  UTC TIME 
    mySerial.write(0x1F);   // Set cursor
    mySerial.write(0x24);   // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(2);      // position y

                            // print UTC TIME on VFD with nice 0 when only one digit
    niceTime(utcHour);      // local time offset.
    mySerial.write(':');
    niceTime(localMinutes);
    mySerial.write(':');
    niceTime(localSeconds);
                            // Uncomment for debug. Just to test the result
if (debuging == 1)
    {                            
    Serial.print(utcHour);
    Serial.print(":");
    Serial.print(localMinutes);
    Serial.print(":");
    Serial.println(localSeconds);
    }
           }


// Print Local time on VFD
   
void loctimeVFD(){    
// force a neat display. After a while, the display go weird
// this force the correct display
//    fixedVFD();
    dateVFD();
// set cursor for  Local TIME 
    mySerial.write(0x1F);    // Set cursor
    mySerial.write(0x24);    // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(1);       // position y

// print UTC TIME on VFD with nice 0 when only one digit
    niceTime(localHour);
    mySerial.write(':');
    niceTime(localMinutes);
    mySerial.write(':');
    niceTime(localSeconds);

if (debuging == 1)
    {
    Serial.print(localHour);
    Serial.print(":");
    Serial.print(localMinutes);
    Serial.print(":");
    Serial.println(localSeconds);
    }      }


// Common function to format hour and minutes

void niceTime(int number) {
     if (number >= 0 && number < 10)
         {
         mySerial.write('0');
         }
                                                   mySerial.print(number);
}


// Print DATE DD-MMM-YYYY on the upper line of VFD


void dateVFD(){
     tmElements_t tm;
     RTC.read(tm);     
 
                            // set cursor for  DATE   DD MMM YY 
    mySerial.write(0x1F);   // Set cursor
    mySerial.write(0x24);   // Set Cursor
    mySerial.write(10);      // position x
    mySerial.write(1);      // position y
                            // Day
        niceTime(tm.Day);
    if (debuging == 1)
    {
      Serial.print("Day: ");
      Serial.println(tm.Day);
    }
        

    if (debuging == 1)
    {
      Serial.print("Month: ");
      Serial.println(tm.Month);
    }
        
                            // make Month name from Month number     
    switch(tm.Month){
      case 1: 
      mySerial.print("-Jan-");
      break;
      case 2: mySerial.print("-Feb-");
      break;
      case 3: mySerial.print("-Mar-");
      break;
      case 4: mySerial.print("-Apr-");
      break;
      case 5: mySerial.print("-May-");
      break;
      case 6: mySerial.print("-Jun-");
      break;
      case 7: mySerial.print("-Jul-");
      break;
      case 8: mySerial.print("-Aug-");
      break;
      case 9: mySerial.print("-Sep-");
      break;
      case 10: mySerial.print("-Oct-");
      break;
      case 11: mySerial.print("-Nov-");
      break;
      case 12: mySerial.print("-Dec-");
      //default: mySerial.print("/ERR/");
      break; }
                            // Year
      mySerial.println(tmYearToCalendar(tm.Year));
      if (debuging == 1)
    {
      Serial.print("Year: ");
      Serial.println(tmYearToCalendar(tm.Year));
    }
    }

// Put "UT" and "LT" on the upper line of VFD

void fixedVFD(){
    mySerial.write(0x1F);   // Set cursor
    mySerial.write(0x24);   // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(1);      // position y      
      mySerial.write("UT");
   //  
    mySerial.write(0x1F);   // Set cursor
    mySerial.write(0x24);   // Set Cursor
    mySerial.write(19);      // position x
    mySerial.write(1);      // position y  
      mySerial.write("LT");    
}

// Reverse character function - not working. Why?

void reverseChar(){
         mySerial.write(0x1F);
         mySerial.write(0x72);
         mySerial.write(1);
}

// just for fun 
void blinkVFD(){
         mySerial.write(0x1F);
         mySerial.write(0x45);
         mySerial.write(blRate);
}

// Set brightness of the VFD
void howBright(){
         mySerial.write(0x1B);
         mySerial.write(0x2A);
         mySerial.write(bright);
}

// read DHTxx sensor
void readDHT()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h = dht.readHumidity();
  // Read temperature as Celsius
  t = dht.readTemperature();
  // Read temperature as Fahrenheit
  f = dht.readTemperature(true);
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index
  // Must send in temp in Fahrenheit!
  hi = dht.computeHeatIndex(f, h);

  Serial.print("Humidity: "); 
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: "); 
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hi);
  Serial.println(" *F");
}

void hVFD(){      
                            // set cursor for  UTC TIME 
    mySerial.write(0x1F);   // Set cursor
    mySerial.write(0x24);   // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(2);      // position y

    mySerial.write("hum=");
    niceTime(h);
    mySerial.write("% ");
    }

void tVFD(){      
                            // set cursor for  UTC TIME 
    mySerial.write(0x1F);   // Set cursor
    mySerial.write(0x24);   // Set Cursor
    mySerial.write(10);      // position x
    mySerial.write(2);      // position y

    mySerial.write("temp=");
    mySerial.print(t,1);
  //  niceTime(t,1);
    mySerial.write(0xf8);
    mySerial.write("C");
    }
