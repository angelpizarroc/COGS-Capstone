/* Script : PotholeEarlyWarning.ino
 * Author : Liam Gowan
 * Date   : March 26th, 2019
 * Purpose: Code for a pothole early warning device. When within 50 m of a pothole, 
 *          and when traveling in direction of pothole when it was captured, a 
 *          buzzer will sound.
 *
 * References:
 *
 * GPS Module:
 * http://arduiniana.org/libraries/tinygpsplus/
 *
 * SD Card Module:
 * https://randomnerdtutorials.com/guide-to-sd-card-module-with-arduino/
 *
 * Piezo buzzer:
 * https://programmingelectronics.com/an-easy-way-to-make-noise-with-arduino-using-tone/?fbclid=IwAR1teOa4kVXI9iwnuMcvbjpxJBa90iNEoMXnPO8_AwXLkxgzkz54sw4ecu4
 *
 * String splitting (for the commas in each row of .txt file)
 * https://arduino.stackexchange.com/questions/1013/how-do-i-split-an-incoming-string?fbclid=IwAR1k6n7cfzibGKHm77vLPtyfHEHkUNK-uEsl3Db51mZkvgmHzWf9r6ML_h8
 */

#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SD.h>

File myFile;

//pin set up and GPS baud rate
static const int RXPin = 2, TXPin = 3;
static const uint32_t GPSBaud = 9600;
int piezoPin = 8;

//GPS and SoftwareSerial objects
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

//Set up buzzer, Serial connection, GPS, and SD card module
void setup() {
  //test speaker
  soundAlarm();
  Serial.begin(9600); //Begin serial connection
  
  ss.begin(GPSBaud); //Begin connection with GPS

  //Initialize SD Card
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
}

void loop(){
//While loop ensures processing will only take place if GPS has data to be read
  while (ss.available() > 0){
    if (gps.encode(ss.read())){
      if(gps.location.isValid()){
		//if location is valid, open the .txt file containing potholes
        myFile = SD.open("pthles.txt");
        if(myFile){
          while(myFile.available()){ //while not at EOF...
			//get latitude, longitude, and heading from file
            float dataLat = myFile.readStringUntil(',').toFloat();
            float dataLong = myFile.readStringUntil(',').toFloat();
            float dataHeading = myFile.readStringUntil('\n').toFloat();
			//calculate distance in metres
            long distance = (long)TinyGPSPlus::distanceBetween( 
              gps.location.lat(), gps.location.lng(), dataLat, dataLong);
			//Alarm condition, only true if within 50m of a pothole, and heading in it's direction (+/- 30 degrees)
            if(distance<=50 && dataHeading <= getLower(gps.course.deg()) && dataHeading >= getHigher(gps.course.deg())){
              soundAlarm(); 
            }
          }
        }  
        myFile.close();
      }
    }
  }
}

//Function rings buzzer for 2/3 of a second.
void soundAlarm(){
  tone(piezoPin, 1000, 667);
}

//Function will "subtract" 30 from the current heading, but will correct for the minimum of 0 degrees
float getLower(float low){
  if((low-30)<0)
    low = 360+low-30;
  else
    low = low - 30;
  return low;
}

//Function will "add" 30 from the current heading, but will correct for the maximum of 359 degrees
float getHigher(float high){
  if((high+30)>=360)
    high = high + 30 - 360;
  else
    high = high + 30;
  return high;
}
