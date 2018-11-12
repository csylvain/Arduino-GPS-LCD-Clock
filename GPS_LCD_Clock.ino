#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

SoftwareSerial mySerial(10, 11); // RX, TX

// #define DEBUG

#ifdef DEBUG
 #define DEBUG_PRINT(...)  Serial.print(F(#__VA_ARGS__" = ")); Serial.print(__VA_ARGS__); Serial.print(F(" "))
#else
 #define DEBUG_PRINT(...)
#endif
#ifdef DEBUG
 #define DEBUG_PRINTLN(...)  DEBUG_PRINT(__VA_ARGS__); Serial.println()
#else
 #define DEBUG_PRINTLN(...)
#endif

#define VERSION F("v0.22")

void setup() {
  Serial.begin(9600);
  Serial.print(F("\nGPS LCD Clock "));
  Serial.println(VERSION);
  mySerial.begin(9600); // u-blox NEO-8M default baud rate
  
  lcd.init();                      // initialize the lcd 
  // Print a message to the LCD.
  lcd.backlight();
  lcd.print(F("GPS LCD Clock")); // F(str) saves SRAM space when str is a constant
  lcd.setCursor(6, 1);
  lcd.print(VERSION);

  delay(1500);

  lcd.clear();
  
  lcd.home();
  lcd.print(F("GPS Time: "));
  lcd.setCursor(3, 1);
  lcd.print(F("--:--:--"));
  lcd.print(F(" UTC")); // going to reuse the " UTC" string
  
  delay(500);
}

int oldsec = 1, newsec = 0, echo = 0;
String sHH = "", sMM, sSS;
String buf = ""; // $xxGGA,HHMMSS.00,...\0

void loop() {
  char c;
  
  if (mySerial.available()) {
    c = mySerial.read(); // sentence starts with '$' char
    
    if (c == '$') { // need to capture at most the next 16 chars. the next 5 to know which sentence.
      // buf is empty (initialized), or contains a full $xxGGA sentence
      if (buf.length() > 16) {
        sHH = buf.substring(7,9);       
        sMM = buf.substring(9,11);
        sSS = buf.substring(11,13);
        // wait for GPS startup to complete. until then, time field in sentence will be empty.
        if (sMM.indexOf(',') == -1 && sSS.indexOf(',') == -1 && !(oldsec == 1 && newsec == 0))  newsec = sSS.toInt();
      }
      DEBUG_PRINT("buf len = ");
      DEBUG_PRINTLN(buf.length());
      buf = ""; // reset buffer
      buf += c;
      echo = 0;
    }
    else if (buf.length() < 6) {
      buf += c;
    }
    else if (buf.length() == 6) {
      DEBUG_PRINT(buf);
      DEBUG_PRINTLN(c);        
      if (buf.substring(3,6) == F("GGA")) {
        DEBUG_PRINT("Found GGA - ");

        Serial.print(buf); // catch the serial monitor up

        echo = 1; // and echo the rest of the $xxGGA sentence
      }
      else echo = 0;
    }
    
    if (echo) {
      Serial.write(c);
      buf += c; // gather the rest of the $xxGGA sentence for later
    }
  }

  if (sHH.length() > 0 && oldsec != newsec) {
    lcd.setCursor(3, 1);
    lcd.print(sHH+F(":")+sMM+F(":")+sSS+F(" UTC"));
    oldsec = newsec;
  }
  // what can't be done here is: delay(1000); (to leave time for human reading the display)
  // think about it: there's a flood of serial data from the GPS module. must process more right away.
  // need a non-blocking way to update the LCD instead - so wait until the SS changes in HHMMSS
}
