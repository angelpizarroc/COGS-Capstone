/* Script : PotholeAutomatic.ino
 * Author : Liam Gowan
 * Date   : March 23rd, 2019
 * Purpose: Code for an automatic pothole detector. Uses an accelerometer (GY-521) to detect 
 *          changes in Z acceleration, and whenever it passes a certain threshold it will 
 *	        capture location (and other information) using a GPS module (NEO 6M), and log 
 *          to an SD card module. The information written is the form {latitude, longitude, 
 *		    date, time, number of sats, bump measurement, course (direction), and whether 
 *          or not it's verified}. For automatic collection, whether or not it's verified 
 *          is always set to 'N' for No.
 *
 * References:
 * Accelerometer:
 * https://create.arduino.cc/projecthub/Nicholas_N/how-to-use-the-accelerometer-gyroscope-gy-521-6dfc19
 *
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

//The threshold from which potholes are recorded if bump measurement is over this
int bumpThreshold = 7500;

//File to write to
File myFile;

//pin set up and GPS baud rate
static const int RXPin = 2, TXPin = 3;
static const uint32_t GPSBaud = 9600;

//GPS and SoftwareSerial objects
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

//Address for accelerometer, and variable to store acceleration data
const int MPU=0x68; 
int16_t AcZ;

int numSats = 0; // to store the current number of sats

bool potholeStatus = false; //for determining whether or not to write

int16_t diff = 0; // to store bump measurement

//For current and previous date time. Last is stored to determine whether or not 
//it's too early to write another
String currentDateTime="";
String lastDateTime = "01/01/2000,0:00:00";

//Set up accelerometer, Serial connection, GPS, and SD card module
void setup() {
  //Initiate connection with accelerometer
  Wire.begin(); 
  Wire.beginTransmission(MPU); 
  Wire.write(0x6B); 
  Wire.write(0); 
  Wire.endTransmission(true); 

  Serial.begin(9600); //Begin serial connection
  
  ss.begin(GPSBaud); //Begin connection with GPS

  //Initialize SD Card, Serial print statements are for when device is connected
  //  to computer for diagnostics
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
}

//Main loop that handles reading sensor values and writing to SD card
void loop() {
  //While loop ensures processing will only take place if GPS has data to be read
  while (ss.available() > 0){ 

    //Acquire bump measurement if there is no pothole to write, and if there is 
	//at least 5 satellites connected
    if(numSats>=5 && potholeStatus==false){
      diff = getDiff();
    }

    //If bump measurement is over threshold, set write status to true 
	//(and if sats >= 5, and previously false)
    if(numSats>=5 && diff>=bumpThreshold && potholeStatus==false){
     potholeStatus=true;
    }

    //Process if it can read the current GPS value
    if (gps.encode(ss.read())){
      numSats = gps.satellites.value();
      //If there is a pothole to write, write it immediately
      if(gps.location.isValid() && potholeStatus==true){
		  //Calls function to build the string for current date and time properly formatted
        buildCurrentDateTimeString(); 
        if(currentDateTime == lastDateTime){ //If at same second as last pothole, stop writing
          potholeStatus = false;
        }
        else{ //If not at same second as last pothole, continue to write
          //Open file, write all attributes (separated with commas), and close it
          myFile = SD.open("neo_a1.txt", FILE_WRITE);
          if(myFile){
            myFile.print(gps.location.lat(),6);
            myFile.print(",");
            myFile.print(gps.location.lng(),6);
            myFile.print(",");
            myFile.print(currentDateTime);
            myFile.print(",");
            myFile.print(numSats);
            myFile.print(",");
            myFile.print(diff);
            myFile.print(",");
            myFile.print(gps.course.deg());
            myFile.print(",");
            myFile.print("N");
            myFile.print(",");
            myFile.println(gps.speed.kmph());
            myFile.close();
          }
          //Reset variables, and set last dateTime to the current one
          diff = 0;
          potholeStatus = false;
          lastDateTime = currentDateTime;

          //Serial print statement kept for diagnostic purposes
          Serial.println("written to file");
        } 
      }    
    }
  }
}

/* Gets current accelerometer value and computes differences from 16300 
   (the average "stand still value of z acceleration) */
static int16_t getDiff(){
  //Initiate connection
  Wire.beginTransmission(MPU); 
  Wire.write(0x3B); 
  Wire.endTransmission(false); 
  Wire.requestFrom(MPU,14,true); 

  Wire.read()<<8|Wire.read(); //Skip X acc.
  Wire.read()<<8|Wire.read(); //Skip Y acc.
  AcZ=Wire.read()<<8|Wire.read();  //Get Z acceleration
  Wire.read()<<8|Wire.read(); //Skip temperature
  Wire.read()<<8|Wire.read(); //Skip X gyro
  Wire.read()<<8|Wire.read(); //Skip Y gyro
  Wire.read()<<8|Wire.read(); //Skip Z gyro
 
  return abs(16300 - abs(AcZ)); //Get absolute of difference
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