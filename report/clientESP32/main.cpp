#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.hpp>

#define MODE_BOOKSHELF
// #define MODE_READER

#ifdef MODE_READER
#include "mfrc.hpp"
#endif
#ifdef MODE_BOOKSHELF
#include "tapeLed.hpp"
#endif

#include "internet.hpp"
#include "serverGasUrl.hpp"

namespace internet{

constexpr char wifiSsid[]="GAS_IoT";
constexpr char wifiPassword[]="gasIot20230726";

extern LongPolling longPolling;

#ifdef MODE_READER
extern PostData mainStatusPost;
#endif

enum InternetStatus{
  IS_INIT,
  IS_WIFI_RESTART,
  IS_WAIT_WIFI_CONNECT,
  IS_WIFI_CONNECTED,
  IS_RUNNING,
  IS_WAIT_STOP_CONNECTION_TO_RESTART,
};

void setup();
void loop();

String getGasUrlWithType(const char *);

}//namespace internet

#ifdef MODE_READER
namespace mfrc{

extern MFRC522 mfrc522;
extern MfrcHelper mfrc522Helper;

void setup();
void loop();

}//namespace mfrc
#endif

#ifdef MODE_BOOKSHELF
namespace tapeLed{

extern TapeLedHelper tapeLed;

void setup();
void loop();

void command(ArduinoJson::JsonVariantConst);

}//namespace tapeLed
#endif

#ifdef MODE_READER
constexpr byte =26;
void sendUid(const String &uidRef){
  tone(BEEP_PIN,4000,1000);
  ArduinoJson::StaticJsonDocument<100> doc;
  String payload;
  doc["nfc_1_uid"]=uidRef;
  ArduinoJson::serializeJson(doc,payload);
  internet::mainStatusPost.postRequest(payload);
}
void callbackFunc(ArduinoJson::JsonVariantConst json){
  Serial.printf("receive json: %s\n",json.as<String>().c_str());

}
#endif
#ifdef MODE_BOOKSHELF
void callbackFunc(ArduinoJson::JsonVariantConst json){
  Serial.printf("receive json: %s\n",json.as<String>().c_str());
  if(json.containsKey("tapeLed")){
    Serial.println("contains tapeLed !!");
    tapeLed::command(json["tapeLed"]);
  }else{
    Serial.println("no contains tapeLed");
  }
}
#endif

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  internet::setup();
  internet::longPolling.attach(callbackFunc);

  #ifdef MODE_READER
  mfrc::setup();
  tone(BEEP_PIN,4000,1000);
  #endif
  #ifdef MODE_BOOKSHELF
  tapeLed::setup();
  #endif

  delay(3000);
}

void loop() {
  // put your main code here, to run repeatedly:
  internet::loop();

  #ifdef MODE_READER
  mfrc::loop();
  if(mfrc::mfrc522Helper.isReadyNewCard()){
    const String &uidRef=mfrc::mfrc522Helper.refUidString();
    Serial.printf("newcard uid:%s\n",uidRef.c_str());
    sendUid(uidRef);
    mfrc::mfrc522.PICC_HaltA();
  }
  #endif
  #ifdef MODE_BOOKSHELF
  tapeLed::loop();
  #endif

  delay(1);
}




namespace internet{
constexpr byte INTERNET_CHECK_LED_PIN=32;

String getGasUrlWithType(const char *type){
  return String(gasUrl)+"?type="+type;
}


//リトライ回数は3回まで
#ifdef MODE_READER
LongPolling longPolling(getGasUrlWithType("polling"),1);
PostData mainStatusPost(getGasUrlWithType("updateStatus"),3);
#endif
#ifdef MODE_BOOKSHELF
LongPolling longPolling(getGasUrlWithType("polling"),2);
#endif

void setup(){
  longPolling.printDebug();
  longPolling.attach(callbackFunc);
  pinMode(INTERNET_CHECK_LED_PIN,OUTPUT);
  
}
void loop(){
  static InternetStatus status=IS_INIT;
  static OneshotC_ms timer;
  longPolling.check();
  #ifdef MODE_READER
  mainStatusPost.check();
  #endif

  switch(status){
    case IS_INIT:{
      Serial.print("Connecting to ");
      Serial.println(wifiSsid);
      WiFi.begin(wifiSsid, wifiPassword);
      timer.start(30000);
      //wifiが接続されない異常状態が10秒続いたら再起動するために10秒測る
      status=IS_WAIT_WIFI_CONNECT;
      digitalWrite(INTERNET_CHECK_LED_PIN,LOW);
      break;
    }
    case IS_WIFI_RESTART:{
      Serial.println("wifi restart");
      WiFi.disconnect();
      WiFi.reconnect();
      timer.start(30000);
      status=IS_WAIT_WIFI_CONNECT;
    }
    case IS_WAIT_WIFI_CONNECT:{
      if(WiFi.status() == WL_CONNECTED){
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        digitalWrite(INTERNET_CHECK_LED_PIN,HIGH);
        status=IS_WIFI_CONNECTED;
      }else if(timer){
        //タイマーが鳴ったら
        status=IS_WIFI_RESTART;
      }
      break;
    }
    case IS_WIFI_CONNECTED:{
      Serial.println("longPolling started");
      longPolling.start();
      #ifdef MODE_READER
      mainStatusPost.start();
      #endif
      // timer.start(10000);
      status=IS_RUNNING;
      break;
    }
    case IS_RUNNING:{
      //実行時の通常のステータス
      if(!longPolling.isAlive()){
        digitalWrite(INTERNET_CHECK_LED_PIN,LOW);
        Serial.println("detect long polling die");
        longPolling.stopRequest();
        #ifdef MODE_READER
        mainStatusPost.stopRequest();
        #endif
        status=IS_WAIT_STOP_CONNECTION_TO_RESTART;
        Serial.println("waiting long polling stop");
      }
      break;
    }
    case IS_WAIT_STOP_CONNECTION_TO_RESTART:{
      if(
        longPolling.isStop()
        #ifdef MODE_READER
        &&mainStatusPost.isStop()
        #endif
      ){
        status=IS_WIFI_RESTART;
      }
      break;
    }
  }
}


}//namespace internet


#ifdef MODE_READER
namespace mfrc{

// MOSI - 23
// MISO - 19
// SCK - 18
constexpr byte RST_PIN=17;          // Configurable, see typical pin layout above
constexpr byte SS_PIN=16;         // Configurable, see typical pin layout above SDA

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MfrcHelper mfrc522Helper(&mfrc522);

void setup(){
  SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	delay(4);				// Optional delay. Some board do need more time after init to be ready, see Readme
}
void loop(){
}

}//namespace mfrc
#endif

#ifdef MODE_BOOKSHELF
namespace tapeLed{

constexpr size_t LED_NUM=60;
constexpr byte LED_PIN=33;

CRGB leds[LED_NUM];
TapeLedHelper tapeLed(leds,LED_NUM);

void setup(){
  FastLED.addLeds<NEOPIXEL,LED_PIN>(leds,LED_NUM);
  tapeLed.test();
}
void loop(){

}

void command(ArduinoJson::JsonVariantConst cmd){
  if(cmd.containsKey("array")){
    Serial.println("contains array !!");
    auto array=cmd["array"].as<ArduinoJson::JsonArrayConst>();
    tapeLed.clearMemory();
    for(ArduinoJson::JsonVariantConst v : array){
      tapeLed.onMemory(v["num"],v["color"].as<uint32_t>());
    }
    tapeLed.show();
  }
  
}

}//namespace tapeLed
#endif