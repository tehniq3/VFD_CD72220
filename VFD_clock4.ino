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
 * v.3 - changed DS1307RTC library with RTClib.h
 * v.4 - added adjusting using 3 buttons, simmilar as for DPD-201: https://github.com/tehniq3/DPD-201/
 */

#include "DHT.h"  // used Example testing sketch for various DHT humidity/temperature sensors
                  // Written by ladyada, public domain
#include <Wire.h>
#include "RTClib.h"  // possible from here: https://github.com/adafruit/RTClib
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"     Sunday", "     Monday", "    Tuesday", "  Wednesday", "   Thursday", "     Friday", "   Saturday"};
//char daysOfTheWeek[7][12] =   {"   Duminica", "       Luni", "      Marti", "   Miercuri", "        Joi", "     Vineri", "    Sambata"};

#include <SoftwareSerial.h>
SoftwareSerial mySerial(0, 2); // RX, TX
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


 int bright = 1;              // 1 < n < 4
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

int tzeci, tunit, tzecimi, trest;
int tsemn, ttot;
int hzeci, hunit;

int hh, mm, ss;
int yy, ll, dd, zz;

unsigned long tpreglaj = 0; 
unsigned long tpreglajmax = 120000; // maximum time for adjust clock

#define meniu 3     // button MENU 
#define plus  5     // button +
#define minus 4     // button -
int nivel = 20;     // state level 
// 0 - normal clock
// 2 - chamge hour
// 4 - change minute
// 6 - chamge year
// 8 - change month
// 10 - change day
// 20 - ready

//int limba = 1;   // 1 for english, 2 for roumanian  // ToDo
int maxday;


void setup() {
  if (debuging == 1)
  {
  Serial.begin(9600);
  while (!Serial) ; // wait for serial
  delay(200);
  }
  Serial.println("RTC clock on VFD CD7220 display v.4");
  Serial.println("-------------------");

pinMode(meniu, INPUT);   // MENU button
pinMode(plus, INPUT);   // + button
pinMode(minus, INPUT);   // + button
digitalWrite(meniu, HIGH);  // pull-up the pin
digitalWrite(plus, HIGH);   // pull-up the pin
digitalWrite(minus, HIGH);  // pull-up the pin

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
   if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
//rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
// rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

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
         
}  // end setup

void setDispMode(){
  mySerial.write(0x02);  //  STX
  mySerial.write(0x05);  //  05
  mySerial.write(0x43);  //  C
  mySerial.write(0x31);  //  31h - ESC/POS mode
  mySerial.write(0x03);  //  ETX
}

void loop() {


if (nivel == 0)  // clock
{
   if (millis() - tpceas > tpreadceas)
   {
   DateTime now = rtc.now();
   if (debuging == 1)
    {
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    }
hh = now.hour();
mm = now.minute();
ss = now.second();
yy = now.year()-2000;
ll = now.month();
dd = now.day();
zz = now.dayOfTheWeek();
     
    if (ss/10%2 == 0)
    dateVFD();
    else
    dayVFD();
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

if (digitalRead(meniu) == LOW)
  {
  nivel = 1;
  tpreglaj = millis();
  mop();
  delay(500);
  }  
} // end nivel = 0

if (nivel == 1)  // hour adjust
{
// print text on VFD 
    mySerial.write(0x1F);    // Set cursor
    mySerial.write(0x24);    // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(1);       // position y
    mySerial.print("Hour set: ");
    mySerial.print(hh/10);
    mySerial.print(hh%10);

if (digitalRead(meniu) == LOW)
  {
  nivel = 2;
  mop();
  delay(500);
  }

if (digitalRead(plus) == LOW) 
  {
  hh = hh + 1;
  delay(250);
  }
if (digitalRead(minus) == LOW) 
  {
  hh = hh - 1;
  delay(250);
  }
  
if (hh > 23) hh = 0; 
if (hh < 0) hh = 23;  
//mop();
}  // end nivel = 1;   

if (nivel == 2)  // minute adjust
{
// print text on VFD 
    mySerial.write(0x1F);    // Set cursor
    mySerial.write(0x24);    // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(1);       // position y
    mySerial.print("Minute set: ");
    mySerial.print(mm/10);
    mySerial.print(mm%10);

if (digitalRead(meniu) == LOW)
  {
  nivel = 3;
  mop();
  delay(500);
  }

if (digitalRead(plus) == LOW) 
  {
  mm = mm + 1;
  delay(250);
  }
if (digitalRead(minus) == LOW) 
  {
  mm = mm - 1;
  delay(250);
  }
  
if (mm > 59) mm = 0; 
if (mm < 0) mm = 59;  
//mop();
}  // end nivel = 2;  

if (nivel == 3)  // year adjust
{                    
// print text on VFD 
    mySerial.write(0x1F);    // Set cursor
    mySerial.write(0x24);    // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(1);       // position y
    mySerial.print("Year set: 20");
    mySerial.print(yy/10);
    mySerial.print(yy%10);

if (digitalRead(meniu) == LOW)
  {
  nivel = 4;
  mop();
  delay(500);
  }

if (digitalRead(plus) == LOW) 
  {
  yy = yy + 1;
  delay(250);
  }
if (digitalRead(minus) == LOW) 
  {
  yy = yy - 1;
  delay(250);
  }
  
if (yy > 50) yy = 25; 
if (yy < 25) yy = 50;  
//mop();
}  // end nivel = 3;  

if (nivel == 4)  // month adjust
{
// print text on VFD 
    mySerial.write(0x1F);    // Set cursor
    mySerial.write(0x24);    // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(1);       // position y
    mySerial.print("Month set: ");
    mySerial.print(ll/10);
    mySerial.print(ll%10);
    mySerial.print("    ");

if (digitalRead(meniu) == LOW)
  {
  nivel = 5;
  mop();
  delay(500);
  }

if (digitalRead(plus) == LOW) 
  {
  ll = ll + 1;
  delay(250);
  }
if (digitalRead(minus) == LOW) 
  {
  ll = ll - 1;
  delay(250);
  }
  
if (ll > 12) ll = 1; 
if (ll < 1) ll = 12;  
//mop();
}  // end nivel = 4; 

if (nivel == 5)  // day adjust
{

// print text on VFD 
    mySerial.write(0x1F);    // Set cursor
    mySerial.write(0x24);    // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(1);       // position y
    mySerial.print("Day set: ");
    mySerial.print(dd/10);
    mySerial.print(dd%10);
    mySerial.print("    ");

if (digitalRead(meniu) == LOW)
  {
  nivel = 6;
  mop();
  delay(500);
  }
     
if (digitalRead(plus) == LOW) 
  {
  dd = dd + 1;
  delay(250);
  }
if (digitalRead(minus) == LOW) 
  {
  dd = dd - 1;
  delay(250);
  }

// Dawn & Dusk controller. http://andydoz.blogspot.ro/2014_08_01_archive.html
 if (ll == 4 || ll == 5 || ll == 9 || ll == 11) { //30 days hath September, April June and November
    maxday = 30;
  }
  else {
  maxday = 31; //... all the others have 31
  }
  if (ll ==2 && yy % 4 ==0) { //... Except February alone, and that has 28 days clear, and 29 in a leap year.
    maxday = 29;
  }
  if (ll ==2 && ll % 4 !=0) {
    maxday = 28;
  }

if (dd > maxday) dd = 1;   
if (dd < 1) dd = maxday;  
//mop();
}  // end nivel = 5; 

if (nivel == 6)  // day in week adjust
{
// print text on VFD 
    mySerial.write(0x1F);    // Set cursor
    mySerial.write(0x24);    // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(1);       // position y
    mySerial.print("Day name:");
    mySerial.print(daysOfTheWeek[zz]);

if (digitalRead(meniu) == LOW)
  {
  nivel = 7;
  mop();
  delay(500);
  }

if (digitalRead(plus) == LOW) 
  {
  zz = zz + 1;
  delay(250);
  }
if (digitalRead(minus) == LOW) 
  {
  zz = zz - 1;
  delay(250);
  }

if (zz > 6) zz = 0;   
if (zz < 0) zz = 6;  
//mop();
}  // end nivel = 6; 

if (nivel == 7)
{ 
 // January 21, 2014 at 3am you would call:
 rtc.adjust(DateTime(2000+yy, ll, dd, hh, mm, 0));
 //setDS3231time(0, mm, hh, zz, dd, ll, yy);
 mop();
 nivel = 20;   
}

if (nivel == 20)
{
mop();
readDHT();
tpdht = millis();
hVFD();
tVFD();
nivel = 0;
}  // end last level & go to clock level

if(nivel != 0 &&  millis() - tpreglaj > tpreglajmax)  // va intra aici doar dacă nu este in functionare normala
{
mop();
nivel = 0; 
}
              
delay(1);              
}  // end main loop


void mop()  // clear the display
{
  mySerial.write(0x0C);     // Clear ALL
  delay(10);  
  mySerial.write(0x1F);    // Set cursor
  mySerial.write(0x24);    // Set Cursor
  mySerial.write(1);      // position x
  mySerial.write(1);       // position y
  mySerial.print("                    ");
  mySerial.print("                    ");
}


// Print Local time on VFD
   
void loctimeVFD(){    
//    dateVFD();
// set cursor for  Local TIME 
    mySerial.write(0x1F);    // Set cursor
    mySerial.write(0x24);    // Set Cursor
    mySerial.write(1);      // position x
    mySerial.write(1);       // position y

// print UTC TIME on VFD with nice 0 when only one digit
    niceTime(hh);
    mySerial.write(':');
    niceTime(mm);
    mySerial.write(':');
    niceTime(ss);

if (debuging == 1)
    {
    Serial.print(hh);
    Serial.print(":");
    Serial.print(mm);
    Serial.print(":");
    Serial.println(ss);
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
                            // set cursor for  DATE   DD MMM YY 
    mySerial.write(0x1F);   // Set cursor
    mySerial.write(0x24);   // Set Cursor
    mySerial.write(10);      // position x
    mySerial.write(1);      // position y
                            // Day
        niceTime(dd);
    if (debuging == 1)
    {
      Serial.print("Day: ");
      Serial.println(dd);
    }
        

    if (debuging == 1)
    {
      Serial.print("Month: ");
      Serial.println(ll);
    }
        
                            // make Month name from Month number     
    switch(ll){
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
      mySerial.println(yy+2000);
      if (debuging == 1)
    {
      Serial.print("Year: ");
      Serial.println(yy+2000);
    }
    }

void dayVFD(){
                            // set cursor for  Day as name
    mySerial.write(0x1F);   // Set cursor
    mySerial.write(0x24);   // Set Cursor
    mySerial.write(10);      // position x
    mySerial.write(1);      // position y
                            // Day
     mySerial.println(daysOfTheWeek[zz]);
                        
    if (debuging == 1)
    {
      Serial.print("Name of the Day: ");
      Serial.println(daysOfTheWeek[zz]);
    }        
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
