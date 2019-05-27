/* Script : PotholeManual.ino
 * Author : Liam Gowan
 * Date   : May 23rd, 2019
 * Purpose: Code for an manual pothole detector. Uses a manual button to allow a user to 
 *			push when a pothole is passed, and captures location (and other information) 
 *			using a GPS module (Adafruit GPS), and log to an SD card module. The information 
 *          written is the form {latitude, longitude, date, time, number of sats, bump 
 *          measurement, course (direction), and whether or not it's verified}.
 *          For manual collection, whether or not it's verified is always set to 'Y' for Yes.
 *
 * References:
 * GPS Module:
 * http://arduiniana.org/libraries/tinygpsplus/
 *
 * SD Card Module
 * https://randomnerdtutorials.com/guide-to-sd-card-module-with-arduino/
 */

#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>

//File to write to
File myFile;

//pin set up and GPS baud rate
static const int RXPin = 3, TXPin = 2;
static const uint32_t GPSBaud = 9600;

//GPS and SoftwareSerial objects
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

int numSats = 0; // to store the current number of sats

bool potholeStatus = false; //for determining whether or not to write

int16_t diff = 0; // to store bump measurement

//For current and previous date time. Last is stored to determine
// whether or not it's too early to write another
String currentDateTime="";
String lastDateTime = "01/01/2000,0:00:00";

int buttonPin = 4;
bool buttonState = false;

//Set up accelerometer, Serial connection, GPS, and SD card module
void setup() {
  
  
  Serial.begin(9600); //Begin serial connection
  
  ss.begin(GPSBaud); //Begin connection with GPS

  //Initialize SD Card, Serial print statements are for when device 
  //is connected to computer for diagnostics
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  pinMode(buttonPin, INPUT);
}

//Main loop that handles reading sensor values and writing to SD card
void loop() {
  
  //While loop ensures processing will only take place if GPS has data to be read
  while (numSats>=5 && ss.available() > 0){ 

    //Acquire bump measurement if there is no pothole to write, 
	//and if there is at least 5 satellites connected
    if(potholeStatus==false){
      buttonState = !digitalRead(buttonPin);
      Serial.println(buttonState);
      
      if(buttonState==1)
        potholeStatus = true;
      else
        potholeStatus = false;
    }

    //Process if it can read the current GPS value
    if (gps.encode(ss.read())){
      numSats = gps.satellites.value();
      //If there is a pothole to write, write it immediately
      if(gps.location.isValid() &&  potholeStatus==true){
		  //Calls function to build the string for current date and time properly formatted
        buildCurrentDateTimeString(); 
        if(currentDateTime == lastDateTime){ //If at same second as last pothole, stop writing
          potholeStatus = false;
        }
        else{ //If not at same second as last pothole, continue to write
          //Open file, write all attributes (separated with commas), and close it
          myFile = SD.open("ada_m.txt", FILE_WRITE);
          if(myFile){
            myFile.print(gps.location.lat(),6);
            myFile.print(",");
            myFile.print(gps.location.lng(),6);
            myFile.print(",");
            myFile.print(currentDateTime);
            myFile.print(",");
            myFile.print(numSats);
            myFile.print(",");
            myFile.print(-1);
            myFile.print(",");
            myFile.print(gps.course.deg());
            myFile.print(",");
            myFile.println("Y");
            myFile.close();
          }
          //Reset variables, and set last dateTime to the current one
          buttonState = false;
          potholeStatus = false;
          lastDateTime = currentDateTime;

          //Serial print statement kept for diagnostic purposes
          Serial.println("written to file");
        } 
      }    
    }
  }
}

//Build a string showing date and time, properly formatted
static void buildCurrentDateTimeString(){
  currentDateTime = String(zeroPad(gps.date.month()));
  currentDateTime.concat('/');
  currentDateTime.concat(zeroPad(gps.date.day()));
  currentDateTime.concat('/');
  currentDateTime.concat(gps.date.year());
  currentDateTime.concat(',');
  currentDateTime.concat(zeroPad(gps.time.hour()));
  currentDateTime.concat(':');
  currentDateTime.concat(zeroPad(gps.time.minute()));
  currentDateTime.concat(':');
  currentDateTime.concat(zeroPad(gps.time.second()));
}

//Given an integer, function will return a string with appropriate 2 digit zero-padding
static String zeroPad(int x){
  String toReturn = String(x);
  if(x<10) //returns with a zero in front if single digit number
    return "0"+toReturn;
  else //otherwise, returns digit as it was
    return toReturn;
}
