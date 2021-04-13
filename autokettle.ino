
#include <SoftwareSerial.h>
#include<Servo.h>
SoftwareSerial wifiSerial(2, 3);      // 2 to TX of ESP8266. 3 to RX of ESP8266

bool DEBUG = true;   //show more logs
int toast_distance_threshold = 10; //cm
int responseTime = 1000; //communication timeout
int pumpPin = 4;
int servoPin = 9;
int servoUpPosition = 90;
int servoDownPosition = -40;
int baudRate = 9600;
Servo ser;
long personFillTime = 60000;
long personBoilTime = 30000;

void setup() {
  Serial.begin(baudRate);
  pinMode(pumpPin, OUTPUT);
  ser.attach(servoPin);
  ser.write(servoUpPosition);
  wifiSerial.begin(baudRate);
  sendToWifi("AT+CIPMUX=1",responseTime,DEBUG); // might not be needed (debugging red herring?)
  delay(2000);
  Serial.println("setup complete");
}

void loop() {
  digitalWrite(pumpPin, LOW);
  int drinkers = 0;
  while (true){
    String fr = sendPollReq();
    //Serial.println("fr: " + fr);
    char final_response[fr.length()];
    fr.toCharArray(final_response, fr.length());
    /* What the fudge is this? well on the esp8266 chip im using,
       the response starts fine, but then turns gibberish.
       i.e. +IPD,4,548:HTTP/1.1 200 OK
        Server: nginx/0u:921CTpnoe0ceeslH  -cnlo,O ctonFtAN-imcnpniel-ro
        ttyl uvsi{{":

        So the response body is gibberish. But the status code is still OK.
        So I can hackily use that to control logic/behaviour :D

        Not sure where gibberish coming from. maybe from running off uno-power, so too low ampage and cant keep up.
        I have it down to 9600 baud-rate which people say should be fine.

        Or it could be due to the software serial having a maximum buffer size of 64 bytes.
        Maybe the response is coming in faster than can be read out, so it's flooding and overfilling the buffer?
        ....but wouldnt it still be first in first out? so this buffer overrun could break other parts of code....
        but should still get correct ordered message out? maybe it drops characters that would overflow buffer somewhere?

        either way, flashing firmware to higher version so I can try out the wifiEspAT library is next thing to try.
    */

    if(fr.indexOf("201") > 0){
      Serial.println("breaking");
      drinkers = 1;
      break;
    } else if (fr.indexOf("202") > 0){
      Serial.println("breaking");
      drinkers = 2;
      break;
    } else if (fr.indexOf("203") > 0){
      Serial.println("breaking");
      drinkers = 3;
      break;
    }
    else if (fr.indexOf("204") > 0){
      Serial.println("breaking");
      drinkers = 4;
      break;
    }
//        else if (fr.indexOf("444") > 0){
//      Serial.println("breaking");
//      drinkers = 0;
//      break;
    //}
    delay(5000);
  }
  
  Serial.println("filling");
  digitalWrite(pumpPin, HIGH);
  for (int i=0; i<drinkers; i++){
    delay(personFillTime);
  }
  digitalWrite(pumpPin, LOW);
  
  Serial.println("turning on boiler");
  //ser.write(70);
  ser.write(servoDownPosition);
  delay(3000);
  
  Serial.println("moving back up");
  ser.write(servoUpPosition);
  
  delay(personBoilTime * drinkers);
   while (true){ // this loop is to just keep sending confirmation until it got sent successfully.
    String fr = sendReadyReq();
    //Serial.println("fr: " + fr);
    char final_response[fr.length()];
    fr.toCharArray(final_response, fr.length());
    if (fr.length() > 0 && final_response[0] == '\r'){
      Serial.println("breaking");
      break;
    }
    delay(1000);
  }
  while (true){  // just to avoid me unplugging stuff as it somehow "restarts" and manages to pump water when not expected.
    delay(10000);
  }
}

String sendReadyReq(){
  // can we use this instead https://github.com/jandrassy/WiFiEspAT/blob/master/examples/Basic/WebClient/WebClient.ino?
  Serial.println("sending ready req");
  sendToWifi("AT+CIPSTART=4,\"TCP\",\"kettle-on.com\",80",responseTime,DEBUG);
  delay(50);
  sendToWifi("AT+CIPSEND=4,77",responseTime,DEBUG);
  delay(50);
  sendToWifi("GET /ready/ HTTP/1.1",responseTime,DEBUG);
  delay(50);
  sendToWifi("Host: kettle-on.com",responseTime,DEBUG);
  delay(50);
  sendToWifi("Content-Type: application/json",responseTime,DEBUG);
  delay(50);
  String out = sendToWifi("",2000,DEBUG);
  return out;
}

String sendPollReq(){
  // can we use this instead https://github.com/jandrassy/WiFiEspAT/blob/master/examples/Basic/WebClient/WebClient.ino?
  Serial.println("sending poll req");
  sendToWifi("AT+CIPSTART=4,\"TCP\",\"kettle-on.com\",80",responseTime,DEBUG);
  delay(50);
  sendToWifi("AT+CIPSEND=4,76",responseTime,DEBUG);
  delay(50);
  sendToWifi("GET /poll/ HTTP/1.1",responseTime,DEBUG);
  delay(50);
  sendToWifi("Host: kettle-on.com",responseTime,DEBUG);
  delay(50);
  sendToWifi("Content-Type: application/json",responseTime,DEBUG);
  delay(50);
  String out = sendToWifi("",2000,DEBUG);
  return out;
}


// copy-pasted from https://create.arduino.cc/projecthub/imjeffparedes/add-wifi-to-arduino-uno-663b9e imjeffparades
/*
* Name: sendToWifi
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/
String sendToWifi(String command, const int timeout, boolean debug){
  //Serial.println("sending:" + command);
  String response = "";
  wifiSerial.println(command); // send the read character to the esp8266
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(wifiSerial.available())
    {
    // The esp has data so display its output to the serial window
    delay(1);
    char c = wifiSerial.read(); // read the next character.
    //Serial.println(c);
    response+=c;
    delay(1);
    }  
  }
  if(debug)
  {
    Serial.println(">>>command start");
    Serial.println(command + ":::" + response);
    Serial.println(">>>resp end");
  }
  return response;
}
