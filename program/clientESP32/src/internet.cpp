#include "internet.hpp"
#include "internetInternal.hpp"


namespace internet{
LongPolling::LongPolling(const String &urlArg,const size_t connectionNum):
  url{urlArg},
  currentVersion{0},
  slpArrayLen{(connectionNum<=SLP_ARRAY_MAX_LEN)?connectionNum:SLP_ARRAY_MAX_LEN},
  jsonDoc{512}
{
}

bool LongPolling::isExistConnectingVersion(uint32_t version){
  for(size_t i=0;i<slpArrayLen;i++){
    if(slpArray[i].refAgentP()->isNoConnect()){
      continue;
    }
    if(slpArray[i].refAgentP()->getTargetStatusVersion()==version){
      return true;
    }
  }
  return false;
}
size_t LongPolling::getNoConnectSLPIndex(){
  for(size_t i=0;i<slpArrayLen;i++){
    if(slpArray[i].refAgentP()->isNoConnect()){
      return i;
    }
  }
  return slpArrayLen;
}

void LongPolling::start(){
  for(size_t i=0;i<slpArrayLen;i++){
    slpArray[i].start(url);
  }
  status=LP_STARTED;
  failCont=0;
}
void LongPolling::stopRequest(){
  for(size_t i=0;i<slpArrayLen;i++){
    slpArray[i].refMainP()->killTaskRequest();
  }
  status=LP_STOP_REQUEST;
}

void LongPolling::check(){
  if(status==LP_INIT){
    return;
  }
  if(status==LP_STOP_REQUEST){
    status=LP_STOP;
    for(size_t i=0;i<slpArrayLen;i++){
      if(!(slpArray[i].refAgentP()->isStop())){
        //一つでも稼働状態なら
        status=LP_STOP_REQUEST;
      }
    }
  }
  // Serial.printf("LP check called\n");
  for(size_t i=0;i<slpArrayLen;i++){
    slpArray[i].check();
    if(slpArray[i].refAgentP()->checkExistNewReceiveData()){
      Serial.printf("LP check new data received\n");
      String *payloadString=slpArray[i].refMainP()->refReceiveDataP();
      if(payloadString==nullptr){
        if(currentTimeout_ms>1000){
          currentTimeout_ms-=100;
        }
        failCont++;
        isOkConnectStart.set(2000);
      }else{
        ArduinoJson::deserializeJson(jsonDoc,*payloadString);
        auto retString=jsonDoc["ret"].as<String>();
        auto statusObject=jsonDoc["status"].as<ArduinoJson::JsonVariantConst>();
        if(retString.equals("change")){
          if(currentVersion<statusObject["version"]){
            currentVersion=statusObject["version"];
            if(callbackFunc){
              callbackFunc(statusObject);
            }
          }
        }
        if(currentTimeout_ms<30*1000){
          currentTimeout_ms+=100;
        }
      }
    }
  }
  if(isOkConnectStart){
    const size_t freeSLPIndex=getNoConnectSLPIndex();
    if(freeSLPIndex!=slpArrayLen){
      //空いているSLPがある時
      Serial.printf("LP chekc there is free SLP:%d\n",freeSLPIndex);
      for(uint32_t offset=0;offset<slpArrayLen;offset++){
        const uint32_t version=currentVersion+offset;
        if(!isExistConnectingVersion(version)){
          slpArray[freeSLPIndex].refAgentP()->sendRequest(
            currentTimeout_ms,version
          );
          isOkConnectStart.checkPoint();
          isOkConnectStart.set(5000);
          break;
        }
      }
    }
  }
  
}
void LongPolling::attach(LongPollingCallbackFunc func){
  callbackFunc=func;
}
bool LongPolling::isAlive(){
  return failCont<4;
}
bool LongPolling::isStop(){
  return (status==LP_STOP)||(status==LP_INIT);
}
void LongPolling::printDebug(){
  Serial.printf("url:%s\n",url.c_str());
}

PostData::PostData(const String &urlArg,const uint8_t maxRetryCountArg):
  url{urlArg},
  maxRetryCount{maxRetryCountArg}
{}
void PostData::postInternal(){
  client.postRequest(sendPayload,5000);
}
void PostData::start(){
  client.startTask(url);
}
void PostData::stopRequest(){
  client.killTaskRequest();
}
void PostData::postRequest(const String &payload){
  retryCount=0;
  sendPayload=payload;
  postInternal();
}
void PostData::check(){
  if(currentInternalVersion!=client.getReceiveDataVersion()){
    currentInternalVersion=client.getReceiveDataVersion();
    String *receivePayload=client.refReceiveDataP();
    if(receivePayload==nullptr){
      if(retryCount<maxRetryCount){
        retryCount++;
        postInternal();
      }
    }
  }
}
bool PostData::isStop(){
  return client.getStatus()==NonBlockHttpClient::NBC_INIT;
}

}