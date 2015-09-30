#include <EEPROM.h>  
#include <SPI.h>      // RC522 Module uses SPI protocol
#include <MFRC522.h>   // Library for Mifare RC522 Devices
#include <Wire.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);

// the two arrays that store RFID tags
byte masterCard[] = {'31', '17', 'E5', '2B'};
byte currentCard[4];

void setup(){
  Serial.begin(9600);
  // initialize the RFID module
  SPI.begin();
  mfrc522.PCD_Init();
  // Initialize the wire transmission between the two arduinos
  Wire.begin(4); // join i2c bus with address #4
  // On request from the master, the slave (this Arduino code) will call the requestEvent function
  Wire.onRequest(requestEvent); 
}

void loop(){
  // If a RFID tag is tapped to the module then enter if statement
  if(mfrc522.PICC_IsNewCardPresent()){
    // Read the scanned card from the Serial Monitor and store into the currentCard array
    if(mfrc522.PICC_ReadCardSerial()){      
      byte card[4];
      for(int i = 0; i < 4; i++){
        currentCard[i]= mfrc522.uid.uidByte[i];
      }
    }
  }
}
/* This function is called, whenever the Master Arduino sends interrupt request
  to ask for the latest rfid tag scanned (current tag) */
void requestEvent(){
  Serial.println("ASKED");
  // We send the master the latest RFID tag stored in currentCard array
  for(int i = 0; i < 4; i++){
    Wire.write(currentCard[i]);
    Serial.println(currentCard[i], HEX);
  }
}
