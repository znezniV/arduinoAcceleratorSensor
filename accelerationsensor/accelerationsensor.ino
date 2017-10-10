#include <Wire.h> // Include the Wire library
#include <MMA_7455.h> // Include the MMA_7455 library
#include <math.h>

#include <Bridge.h>
#include <BridgeClient.h>
#include <MQTTClient.h>

BridgeClient net;
MQTTClient client;

unsigned long lastMillis = 0;

MMA_7455 mySensor = MMA_7455(); // Make an instance of MMA_7455
char xVal, yVal, zVal; // Variables for the values from the sensor
float lastPoint;
float highPoint = 0;
float lowPoint = 0;
float curveHeight;
int mappedCurveHeight;
boolean peaked = false;
int curveCount = 0;
unsigned long  theTime;
unsigned long  lastTime = 0;
unsigned long  timeSince;
int mappedTimeSince;
int lastSentVelocitiy;
int lastSentPitch;

void setup() {
  Bridge.begin();
  Serial.begin(9600);

  // Set the sensitivity you want to use
  // 2 = 2g, 4 = 4g, 8 = 8g
  mySensor.initSensitivity(2);
  // Calibrate the Offset, that values corespn
  // flat position to: xVal = -30, yVal = -20, zVal = +20
  // !!!Activate this after having the first values read out!!!
  mySensor.calibrateOffset(8, 20, 0);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported by Arduino.
  // You need to set the IP address directly.
  client.begin("broker.shiftr.io", net);
  client.onMessage(messageReceived);
  connect();
}

void connect() {
  Serial.print("connecting...");
  while (!client.connect("accelormeter-in-midi", "44fd6874", "bbbb625576bbaa3f")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  // client.subscribe("/mididata");
  // client.unsubscribe("/hello");
}


void loop() {

  theTime = millis();

  // set variables
  xVal = mySensor.readAxis('x'); // Read out the ‘x’ Axis
  yVal = mySensor.readAxis('y'); // Read out the ‘y’ Axis
  zVal = mySensor.readAxis('z'); // Read out the ‘z’ Axis

  float vCalc = sqrt((xVal * xVal)+(yVal*yVal)+(zVal*zVal));
  //calibrate vCalc
  vCalc = vCalc - 62;

  // check if number
  if (!isnan(lastPoint)) {
    if ((lastPoint < 0 && vCalc >= 0) || (lastPoint <= 0 && vCalc > 0) ) {
      curveHeight = highPoint - lowPoint;
      mappedCurveHeight = round(map(curveHeight, 0, 150, 0, 127));
      lowPoint = 0;
      highPoint = 0;

      timeSince = theTime - lastTime;
      mappedTimeSince = round(map(timeSince, 100, 500, 48, 58));
      lastTime = theTime;
      int lastSentPitch = mappedTimeSince;
      int lastSentVelocitiy = mappedCurveHeight;

      // Serial.print("mappedTimeSince: ");
      // Serial.println(mappedTimeSince);
      // Serial.print("mappedCurveHeight: ");
      // Serial.println(mappedCurveHeight);
    }

    if(vCalc > 0) {
      if (vCalc > highPoint) {
        highPoint = vCalc;
      }
    }
    else if(vCalc < 0){
      if (vCalc < lowPoint) {
        lowPoint = vCalc;
      }
    }
  }

  client.loop();
  if(!client.connected()) {
    connect();
  }

  String midiData = String(mappedTimeSince) + "," + String(mappedCurveHeight);

  // publish a message roughly every second.
  if(millis() - lastMillis > 10) {
    lastMillis = millis();
    client.publish("/vibration", midiData);
  }
  // Serial.println(vCalc);
  lastPoint = vCalc;


}



void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();
}
