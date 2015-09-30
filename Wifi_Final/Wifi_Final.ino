#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include <Wire.h>
#include <Servo.h>
// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER); // you can change this clock speed
#define WLAN_SSID       "aP"           // cannot be longer than 32 characters!
#define WLAN_PASS       ""
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_UNSEC
#define IDLE_TIMEOUT_MS  500      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.
// What page to grab!
#define WEBSITE      "thawing-reef-6342.herokuapp.com"
#define WEBPAGE      "/dashboard"
// The RFID tag codes
#define TAG1 "E493EA7F"  
#define TAG2 "D460EA7F"
#define TAG3 "448AE87F"
#define MIN_REFRESH_DELAY 5000
// Pins for all the House's hardware
#define LED1_PIN 8
#define LED2_PIN 7
#define LED3_PIN 2
#define DOOR_PIN A2
// Parser Variables 
int LED1, LED2, LED3, DOOR, GDOOR;
String LCD;
Servo garageDoor;
int pos = 0; 
Adafruit_CC3000_Client www;
unsigned long lastRead;
uint32_t ip;
int last_get = 0;
String currentUserRFID;
int currentDoorState;
int last_time = 0;

void setup(void)
{
  // Initialize alll the LED pins
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  // Initialize the Serial Monitor 
  Serial.begin(115200);
  // Setup connection for the Wifi module
  getConnection();  
  connectToServer();
  Serial.println("Connected"); // for debugging purposes
 
  // Setup communication with the other arudino (the Slave)
  Wire.begin(); // join i2c bus with address #4
}
void loop(){
  // Retrieve the RFID tag from the other Arduino (Slave) by requesting data  
    Wire.requestFrom(4, 4);
    String s = "";
    // While the slave arduino is sending data (RFID tag), store it in a String 's'
    while(Wire.available()){
      byte c = Wire.read();
      s += String(c);
    }
    // If we got a new RFID tag then set that as our new 
    if(s != currentUserRFID ){
      Serial.println(s); // for debugging purposes
      //set that the new RFID tag as our new currentUserRfid
        currentUserRFID = s;
      //call the sendRFID function with our new RFID tag 's'
        sendRFID(s);
    }  
  delay(500); // delay the loop by half a second
}
/*  Function to do a Post request to the server givne a rfid tag. 
  Also retrieve data from the post request and store it. */
void sendRFID(String rfid){
    Serial.println("Send rfdi"); // for debugging purposes
    // If we are connected, then attempt to make a POST request to our sever
    if(www.connected()){    
      www.fastrprint(F("POST /getdata HTTP/1.1\r\n"));
      www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); 
      www.fastrprint(F("\r\n"));
      www.fastrprint(F("Content-Type: application/x-www-form-urlencoded\r\n"));
      www.fastrprint(F("Content-Length: 13"));
      www.fastrprint(F("\r\n"));
      www.fastrprint(F("\r\n"));
      www.fastrprint(F("rfid="));
    // if the current rfid tag matches "85255255255" then we send TAG1
  if(rfid == "85255255255"){
      www.fastrprint(F(TAG1));
    }else if(rfid == "43255255255"){
        // if the current rfid tag matches "43255255255" then we send TAG2
        www.fastrprint(F(TAG2));
    }else if(rfid == "127255255255"){
        // if the current rfid tag matches "127255255255" then we send TAG1
        www.fastrprint(F(TAG3));
    }
    // Print new lines
    www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    }else{
        Serial.println("not connected for post"); // for debugging puposes
        connectToServer(); // Try Connecting to the server again beacuse it failed earlier
        sendRFID(rfid); // Try sending calling the sendRFID function again because it failed earlier
    } 
    String data; // initialize a string called 'data'
  int haveBracket = 0; // initalize a haveBracket flag to 0
  
    lastRead = millis(); // Store current time in lastRead variable, used to see when we last read a rfid tag
    // while are still connected and the time difference between current time and the last time we read a rfid tag is less than IDLE_TIMEOUT_MS
    // what that means, we will retrieve data from the server until we are connected and until IDLE_TIMEOUT_MS time as elapsed from the time
    // we last read a rfid tag
    while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
      // while server is still available read the data and store it in character 'c'
        while (www.available()){
          char c = www.read();
          Serial.print(c);
          if(c == '<'){
            haveBracket = 1; // turn the haveBracket to 1 if 'c = <'
          }
          // if a bracket was found that append that character 'c' to our string 'data'
          if(haveBracket){
            data += c;
          }
          lastRead = millis(); // set our lastRead time to current time
      }
    } 
    // Serial.println("------------------------------DATA STRING ----------------------"); // for debugging purposes
    Serial.println(data); // for debugging purposes
    //Unless the data is an empty string, the parser will be called
    if(data  != ""){
      parser(data);
    }
    //Serial.println("------------------------------END DATA STRING-------------------"); // for debugging purposes
}
/* Function that connects to a server */
void connectToServer(){
    Serial.println("new connection");
    www = cc3000.connectTCP(ip, 80);  
}
/* Function that attempts to connect to a WiFi Network */
void getConnection(){
    Serial.println(F("Hello, CC3000!\n")); // for debugging purposes
    Serial.print("Free RAM: "); // for debugging purposes
    Serial.println(getFreeRam(), DEC); // Print out the current free RAM
    /* Initialise the module */
    Serial.println(F("\nInitializing..."));
    if (!cc3000.begin()){
      Serial.println(F("Couldn't begin()! Check your wiring?")); // for debugging purposes
    }
    Serial.print(F("\nAttempting to connect to ")); // for debugging purposes
    Serial.println(WLAN_SSID);
    // if wifi module cannot connect to the network, print it out it failed for debugging purposes
    if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
        Serial.println(F("Failed!")); 
    }
    Serial.println(F("Connected!")); // for debugging purposes
  
    /* Wait for DHCP to complete */
    Serial.println(F("Request DHCP")); // for debugging purposes
    // while the wifi module doesn't get an IP address, we dealy for 100 ms
    while (!cc3000.checkDHCP()){
      delay(100);
    }  
    /* Display the IP address DNS, Gateway, etc. */  
    while (! displayConnectionDetails()) {
      delay(1000);
    }
    ip = 0;
    // Try looking up the website's IP address
    Serial.print(WEBSITE); Serial.print(F(" -> ")); // for debugging purposes
    // if wifi module doesn't get a proper IP address and a propoer Host then, we print out an error and delay.
    while (ip == 0) {
      if (! cc3000.getHostByName(WEBSITE, &ip)) {
          Serial.println(F("Couldn't resolve!"));
      }
      delay(500);
    }
    cc3000.printIPdotsRev(ip);    
}
/* Closes the CC3000's connection */ 
void closeConnection(){
    www.close(); //closes the connection
    Serial.println(F("-------------------------------------")); //for debugging purposes
    Serial.println(F("\n\nDisconnecting")); //for debugging purposes
    cc3000.disconnect(); // disconnects the wifi module from the network
}
/* Tries to read the IP address and other connection details */
bool displayConnectionDetails(void){
    uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
    if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv)){
      Serial.println(F("Unable to retrieve the IP Address!\r\n"));
      return false;
    }
    else{
      Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
      Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
      Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
      Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
      Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
      Serial.println();
      return true;
    }
}
/* Function that parses the incoming xml string */
void parser(String inputString){
    /* Parses the information for the first light */
    int led1_open = inputString.indexOf("<led1>");
    int led1_close = inputString.indexOf("</led1>");
    String led1_state = inputString.substring(led1_open + 6, led1_close);
    LED1 = led1_state.toInt();
  
    /* Parses the information for the second light */
    int led2_open = inputString.indexOf("<led2>");
    int led2_close = inputString.indexOf("</led2>");
    String led2_state = inputString.substring(led2_open + 6, led2_close);
    LED2 = led2_state.toInt();
  
    /* Parses the information for the third light */
    int led3_open = inputString.indexOf("<led3>");
    int led3_close = inputString.indexOf("</led3>");
    String led3_state = inputString.substring(led3_open + 6, led3_close);
    LED3 = led3_state.toInt();
  
    /* Parses the infomation for the garage door */
    int gdoor_open = inputString.indexOf("<door>");
    int gdoor_close = inputString.indexOf("</door>");
    String gdoor_state = inputString.substring(gdoor_open + 6, gdoor_close);
    GDOOR = gdoor_state.toInt();
  
    /* Prints the status of the lights and garage door for debugging purposes */
    Serial.println(inputString);
    Serial.println(LED1);
    Serial.println(LED2);
    Serial.println(LED3);
    Serial.println(GDOOR);
  
    updateHouse(); // Updates the lights and garage door
}
/* Function that writes the LEDs on or off, and turns the servo motor */ 
void updateHouse(){
  /* Turns the appropriate LEDs on or off */ 
  analogWrite(LED1_PIN, LED1);
  analogWrite(LED2_PIN, LED2);
  analogWrite(LED3_PIN, LED3);
  
  /* Turns the servo motor +180 degrees (opens the garage door) */
 if(GDOOR == 1 && currentDoorState != 1){
    garageDoor.attach(DOOR_PIN);      // Initializes the servo motor (garage door)
    Serial.println("Opening Door");
    for(pos = 0; pos < 180; pos += 1)     // goes from 0 degrees to 180 degrees 
    {                                     // in steps of 1 degree 
      garageDoor.write(pos);                // tell servo to go to position in variable 'pos' 
      delay(15);                          // waits 15ms for the servo to reach the position 
    } 
    currentDoorState = 1;         // Makes sure the servo doesn't try to move if the user doesn't change the status of the garage door
    garageDoor.detach();          // De-initializes the servo motor
  } 
  /* Turns the servo motor -180 degrees (closes the garage door) */
  else if (GDOOR == 0 && currentDoorState != 0) {
    garageDoor.attach(DOOR_PIN);      // Initializes the servo motor (garage door)
    Serial.println("Closing Door");
    
    for(pos = 180; pos>=1; pos-=1)        // goes from 180 degrees to 0 degrees 
    {                                
      garageDoor.write(pos);                // tell servo to go to position in variable 'pos' 
      delay(15);                    // waits 15ms for the servo to reach the position 
    } 
    
    currentDoorState = 0;         // Makes sure the servo doesn't try to move if the user doesn't change the status of the garage door
    garageDoor.detach();          // De-initializes the servo motor
  }
  
}
