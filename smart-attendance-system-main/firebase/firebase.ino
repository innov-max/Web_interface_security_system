#include <ESP8266WiFi.h>
#include <Wire.h> 
#include <SPI.h> 
#include <RFID.h>
#include "FirebaseESP8266.h"  // Install Firebase ESP8266 library
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <BlynkSimpleEsp8266.h>

#define FIREBASE_HOST "rfid-attendance-4a479-default-rtdb.firebaseio.com" //Without http:// or https:// schemes
#define FIREBASE_AUTH "BIPUdNgoeJgiv7jF6n2AqiyRCQJTNmXs42fPEl8R"
#define BLYNK_TEMPLATE_ID "TMPL2ZbW5ut7e"
#define BLYNK_TEMPLATE_NAME "IamIn"
#define BLYNK_AUTH_TOKEN "pxCqt3gaLJCfFDCSbO0VRFdl2JYhZf-_"



RFID rfid(D8, D0);       //D10:pin of tag reader SDA. D9:pin of tag reader RST 
unsigned char str[MAX_LEN]; //MAX_LEN is 16: size of the array 


WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 19800; //(UTC+5:30)
NTPClient timeClient(ntpUDP, "pool.ntp.org");


const char ssid[] = "TROJAN";
const char pass[] = "Alex@7402";

String uidPath= "/";
FirebaseJson json;
//Define FirebaseESP8266 data object
FirebaseData firebaseData;

unsigned long lastMillis = 0;
const int red = D4;
const int green = D3;
String alertMsg;
String device_id="Engineering";
boolean checkIn = true;

BlynkTimer timer;


void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\n connected!");
}

void setup()
{

  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  

  //pinMode(red, OUTPUT);
  //pinMode(green, OUTPUT);
              
  SPI.begin();
  rfid.init();
  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);
  connect();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Blynk.begin(BLYNK_AUTH_TOKEN,ssid,pass);
  
  Blynk.logEvent("temp_alert","Temp above 30 degree");

}
void checkAccess (String temp)    //Function to check if an identified tag is registered to allow access
{
    
    if(Firebase.getInt(firebaseData, uidPath+"/users/"+temp)){
      
      if (firebaseData.intData() == 0)         //If firebaseData.intData() == checkIn
      {  
        Blynk.run();
        timer.run();
          alertMsg="CHECKING IN";
          delay(1000);

          json.add("time", String(timeClient.getFormattedDate()));
          json.add("id", device_id);
          json.add("uid", temp);
          json.add("status",1);

          Firebase.setInt(firebaseData, uidPath+"/users/"+temp,1);
          
          if (Firebase.pushJSON(firebaseData, uidPath+ "/attendence", json)) {
            Serial.println(firebaseData.dataPath() + firebaseData.pushName()); 
          } else {
            Serial.println(firebaseData.errorReason());
          }
      }
      else if (firebaseData.intData() == 1)   //If the lock is open then close it
      { 
        
          alertMsg="CHECKING OUT";
          delay(1000);
         

          Firebase.setInt(firebaseData, uidPath+"/users/"+temp,0);
          
          json.add("time", String(timeClient.getFormattedDate()));
          json.add("id", device_id);
          json.add("uid", temp);
          json.add("status",0);
          if (Firebase.pushJSON(firebaseData, uidPath+ "/attendence", json)) {
            Serial.println(firebaseData.dataPath() + firebaseData.pushName()); 

            
            
          } else {
            Serial.println(firebaseData.errorReason());
          }
      }
 
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + firebaseData.errorReason());
    }
}
void loop() {
  timeClient.update();
  if (rfid.findCard(PICC_REQIDL, str) == MI_OK)   //Wait for a tag to be placed near the reader
  { 
    Serial.println("Card found"); 
    String temp = "";                             //Temporary variable to store the read RFID number
    if (rfid.anticoll(str) == MI_OK)              //Anti-collision detection, read tag serial number 
    { 
      Serial.print("The card's ID number is : "); 
      for (int i = 0; i < 4; i++)                 //Record and display the tag serial number 
      { 
        temp = temp + (0x0F & (str[i] >> 4)); 
        temp = temp + (0x0F & str[i]); 
      } 
      Serial.println (temp);
      checkAccess (temp);     //Check if the identified tag is an allowed to open tag
    } 
    rfid.selectTag(str); //Lock card to prevent a redundant read, removing the line will make the sketch read cards continually
  }
  rfid.halt();

}
