#include <GSM.h>
#include "SparkFunLIS3DH.h" 
#include "Wire.h"
#include "SPI.h"

//Text message variables//
const char ID_BIN[] = "000000";

///IR Sensor Variables//
int irSensorPin = 0; //analog pin 0, IR Sensor output
const int initIterations = 10; //The number of times we check distance for our initial distastance
int currentIteration = 0; 
int distanceInitArray[initIterations]; //Holds all initial distance values
int distanceInitVal = 0; //This is the value that we measure against when checking if the bin is full
boolean irInitialised = false; //True when we have our distanceInitVal

///GSM Variables///
GSM gsmAccess;
GSM_SMS sms;
GSMScanner scan;

const char numServiceProvider[] = "+447494801240"; //Service provider sms target. Currently set to a personal number for testing
char numUser[] = "+447494801240"; //Mobile number for the service reciever. Currently set to a personal number for testing
boolean notConnected = true; //tracking GSM connection status
boolean sentRequestNewSMS = false; //True after we have sent an SMS message to request a new bin 
boolean sentIncorrectOrientationSMS = false; //True after we have sent an SMS message informing the user that the bin has been knocked over or dropped

///ACCELEROMETER VARIABLES///
LIS3DH myIMU; //LIS3DH Accelerometer

void setup(){
  Serial.begin(9600);
  Serial.println("starting gsm scanner...");
  scan.begin();
  Serial.println("beginning gsmAccess...");
  gsmConnect();
  Serial.println("starting accelerometer...");
  myIMU.begin();
}

void loop(){
  
  if(!irInitialised){
    irInit();
  }
  else{
    if(!sentRequestNewSMS){ //Check if the bin is full only if we already havent requested a new one
      checkBinFull();
    }
    
    if(checkIncorrectOrientation()){
       delay(2000); //Wait 5 seconds then check again in case the bin is being intentionally moved
       if(checkIncorrectOrientation() && !sentIncorrectOrientationSMS){
          String msgSMS = "IoT Sharps bin is not oriented correctly";
          gsmSend(msgSMS, "USER");  
          sentIncorrectOrientationSMS = true;
       }
    }
    else if(fallCheck() && !sentIncorrectOrientationSMS){
      String msgSMS = "IoT Sharps bin has fallen";
      gsmSend(msgSMS, "USER");
      sentIncorrectOrientationSMS = true;
    }
  }
  delay(100);
}

void checkBinFull(){
  if(irInitialised && irCheckFull()){
    delay(1000); //Wait for the bin's contents to settle and double check
    if(irCheckFull()){ //If the sensor is still reporting full after our second check, we can reasonably assume that the bin is full
      Serial.println("GSM NOTIFY FULL");
      String REQ_PICKUP = "REQUEST PICKUP: ";
      String msgSMS = REQ_PICKUP + ID_BIN;
      gsmSend(msgSMS,"SERVICE_PROVIDER");
      sentRequestNewSMS = true;
    }
  }
  else if(!irInitialised){
    irInit(); //initialise if we havent already
  }
}

///IR FUNCTIONS///
/*
 * Handles the initialisation of the IR sensor.
 * Sets up a value from which to base our distance measurements on
 */
void irInit(){
  if(currentIteration < initIterations){
    int val = analogRead(irSensorPin);
    distanceInitArray[currentIteration] = val;    
    currentIteration = currentIteration + 1; 
    Serial.println(val);
  }
  else{
    Serial.println("not initialised");
    int totalInitVals = 0; 
    
    for(int i = 0; i<initIterations; i++){
      totalInitVals += distanceInitArray[i]; //Sum up all initial measurements
    }

    distanceInitVal = totalInitVals/initIterations; //get an average of the measurements
    String lblDistanceInit = "Distance Init Val: ";
    String lblFull = lblDistanceInit + distanceInitVal;
    Serial.println(lblFull);
    irInitialised = true; //Now we can use the bin
  }
}

/**
 * True if the voltage measured at call time is greater than the voltage measured during initialisation                                                                                                                                                                                                                                                                                                                                                                  
 */
boolean irCheckFull(){
  int distanceCurrent = analogRead(irSensorPin);
//  Serial.println(distanceCurrent);
  if(distanceCurrent > distanceInitVal+50){
    return true;
  }
  else{
    return false;
  }
}
///GSM FUNCTIONS///
/**
 * Creates our GSM connection when called
 */
void gsmConnect(){
  while(notConnected){
    if(gsmAccess.begin() == GSM_READY){
      notConnected = false;
      Serial.println("gsm connected!");
    }
    else{
      Serial.println("gsm connecting...");
    }
  }
}

/**
 * Sends an sms to the phone number defined in numServiceProvider[]
 * requesting a new bin. 
 */
void gsmSend(String message, String target){
  
  Serial.println("Sending SMS");
  
  // Starting SMS transmission
  if(target.equals("SERVICE_PROVIDER")){
    sms.beginSMS(numServiceProvider);  
  } 
  else if(target.equals("USER")){
    sms.beginSMS(numUser);
  }
  
  sms.print(message);
  sms.endSMS();
  Serial.println("Sent SMS");
}

///ACCELEROMETER FUNCTIONS///

/**
 * Returns true if the accelerometer reads a G force higher than 1.5G, suggesting an impact has occured
 */
boolean fallCheck(){
  float xVal = myIMU.readFloatAccelX();
  if(xVal > 1.5f){
    return true;
  }
  else{
    return false;
  }
}

/**
 * Returns true if the container is at an incorrect orientation (AccelX > -0.7)
 */
boolean checkIncorrectOrientation(){

  int index = 0; //Used when getting the axis of orientation
  char orientation = 'n'; //whether the axis orientation of thedevice is up(+) or down(-)
  char axis = 'n'; //letter label of axis (x, y, z)
  
  //Gather up accelerometer data
  float xVal = myIMU.readFloatAccelX();
  
  if(xVal > -0.7f){ //If the device is not correctly oriented the FloatAccellY value will be greater than -0.7
//    Serial.println("wrong orientation");
    return true;
  }
  else{
//    Serial.println("correct orientation");
    sentIncorrectOrientationSMS = false; //Reset the sms after the orientation is corrected again
    return false;
  }
}

void accelerometerReadout(){
    Serial.print("\nAccelerometer:\n");
    Serial.print(" X = ");
    Serial.println(myIMU.readFloatAccelX(), 4);
    Serial.print(" Y = ");
    Serial.println(myIMU.readFloatAccelY(), 4);
    Serial.print(" Z = ");
    Serial.println(myIMU.readFloatAccelZ(), 4);
}
