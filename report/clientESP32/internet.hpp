#ifndef INTERNET_HPP_INCLUDE_GUARD
#define INTERNET_HPP_INCLUDE_GUARD

#include <Arduino.h>
#include <HTTPClient_mod.h>
#include <WiFi.h>
// #include <HTTPSRedirect.h>

#include <ArduinoJson.hpp>

#include "internetInternal.hpp"

namespace internet{

//https://iotdesignpro.com/articles/esp32-data-logging-to-google-sheets-with-google-scripts

class OneshotC_ms{
  private:
  unsigned long startTime;
  unsigned long intervalTime;
  bool isRunning;
  public:
  OneshotC_ms():
  startTime(0),intervalTime(0),isRunning(false){

  }
  void start(unsigned long interval){
    intervalTime=interval;
    startTime=millis();
    isRunning=true;
  }
  bool ist(){
    if(isRunning){
      if(intervalTime<(millis()-startTime)){
        isRunning=false;
        return true;
      }
    }
    return false;
  }
  operator bool(){return ist();}
};
class AfterC_ms{
  private:
  unsigned long startTime;
  unsigned long intervalTime;
  public:
  AfterC_ms():
  startTime(0),intervalTime(0){
  }
  void checkPoint(){
    startTime=millis();
  }
  void set(unsigned long interval){
    intervalTime=interval;
  }
  bool ist(){
    if(intervalTime<(millis()-startTime)){
      return true;
    }
    return false;
  }
  operator bool(){return ist();}
};

using LongPollingCallbackFunc=std::function<void(ArduinoJson::JsonVariantConst)>;
class LongPolling{
public:
  enum Status{
    LP_INIT,
    LP_STARTED,
    LP_STOP_REQUEST,
    LP_STOP,
  };
private:
  String url;
  AfterC_ms isOkConnectStart;
  uint32_t currentVersion;
  static constexpr size_t SLP_ARRAY_MAX_LEN=2;
  const size_t slpArrayLen;
  SingleLongPollingTotal slpArray[SLP_ARRAY_MAX_LEN];
  LongPollingCallbackFunc callbackFunc;
  ArduinoJson::DynamicJsonDocument jsonDoc;
  uint32_t currentTimeout_ms=30*1000;
  uint32_t failCont=0;
  Status status=LP_INIT;

  bool isExistConnectingVersion(uint32_t version);
  size_t getNoConnectSLPIndex();
public:
  LongPolling(const String &urlArg,const size_t connectionNum);
  void start();
  void stopRequest();
  void check();
  void attach(LongPollingCallbackFunc);
  bool isAlive();
  bool isStop();
  void printDebug();
};

class PostData{
private:
  const String url;
  const uint8_t maxRetryCount;
  uint8_t retryCount;
  NonBlockHttpClient client;
  uint32_t currentInternalVersion=0;
  String sendPayload;
  void postInternal();
public:
  PostData(const String &,const uint8_t maxRetryCountArg);
  void start();
  void stopRequest();
  void postRequest(const String &);
  void check();
  bool isStop();
};

}//namespace internet

#endif