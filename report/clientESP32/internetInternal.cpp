#include "internetInternal.hpp"


namespace internet{

void longPollingTask(void *longPollingTaskStartInfo){
  longPollingTaskCore(longPollingTaskStartInfo);
  vTaskDelete(nullptr);
}
void longPollingTaskCore(void *longPollingTaskStartInfo){
  const LongPollingTaskStartInfo startInfo=
    *(static_cast<LongPollingTaskStartInfo*>(longPollingTaskStartInfo));
  HTTPClient_mod connection;
  LongPollingTaskComInfo comInfo;
  RegularC_ms printTime(500);
  while(true){
    const BaseType_t ret=xQueueReceive(
      startInfo.motherToChild,
      &comInfo,
      0
    );
    if(printTime){
      Serial.printf("I am live! ptr:%p\n",startInfo.childToMother);
    }
    if(ret==pdTRUE){
      switch(comInfo.type){
        case POST_NOW:
        case GET_NOW:{
          const auto *const infoP=static_cast<const PostGetNowInfo*>(comInfo.ptr);
          const TaskTalkType infoType=comInfo.type;
          String *payloadP=infoP->payloadStringP;
          connection.setTimeout(infoP->timeout+3000);
          connection.setConnectTimeout(infoP->timeout+3000);
          if(connection.begin(startInfo.url)){
            connection.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            connection.addHeader("Content-Type", "application/x-www-form-urlencoded");
            int ret;
            if(infoType==POST_NOW){
              // ret=connection.POST(infoP->payload);
              ret=connection.POST(
                (uint8_t *) payloadP->c_str(),
                payloadP->length()
              );
            }else{ //GET_NOW
              ret=connection.GET();
            }
            
            Serial.printf("done connection with code:%d\n",ret);
            if(ret==HTTP_CODE_OK){
              comInfo.type=RECEIVE_DATA;
              comInfo.ptr=new String(connection.getString());
              xQueueSend(
                startInfo.childToMother,
                &comInfo,
                portMAX_DELAY
              );
            }else{
              comInfo.type=FAIL_DATA;
              comInfo.ptr=nullptr;
              xQueueSend(
                startInfo.childToMother,
                &comInfo,
                portMAX_DELAY
              );
            }
          }else{
            comInfo.type=FAIL_BEGIN;
            comInfo.ptr=nullptr;
            xQueueSend(
              startInfo.childToMother,
              &comInfo,
              portMAX_DELAY
            );
          }
          if(infoType==POST_NOW){
            delete payloadP;
          }
          delete infoP;
          break;
        }
        case KILL_TASK:{
          Serial.printf("KillTask Received ptr:%p\n",startInfo.childToMother);
          connection.end();
          comInfo.type=DONE_KILL_TASK;
          comInfo.ptr=nullptr;
          xQueueSend(
            startInfo.childToMother,
            &comInfo,
            portMAX_DELAY
          );
          Serial.printf("bye ptr:%p\n",startInfo.childToMother);
          return;
        }
      }
    }
    delay(1);
  }
}

NonBlockHttpClient::NonBlockHttpClient():
  isStartInfoInit(false)
{
}

void NonBlockHttpClient::setNewReceiveData(String *newData){
  if(receiveData!=nullptr){
    delete receiveData;
  }
  receiveData=newData;
  receiveDataVersion++;
}

void NonBlockHttpClient::startTask(const String urlArg){
  Serial.println("startTask called");
  if(!isStartInfoInit){
    startInfo.url=urlArg;
    const uint8_t QUEUE_LEN=10;
    startInfo.childToMother=xQueueCreate(
      QUEUE_LEN,sizeof(LongPollingTaskComInfo)
    );
    startInfo.motherToChild=xQueueCreate(
      QUEUE_LEN,sizeof(LongPollingTaskComInfo)
    );
    xTaskCreatePinnedToCore(
      longPollingTask,
      "lpTask",
      8192,
      &startInfo,
      10,
      &task,
      // PRO_CPU_NUM
      APP_CPU_NUM
    );
    isStartInfoInit=true;
    status=NBC_NOCONNECT;
    Serial.println("startTask done");
  }else{
    Serial.println("startTask fail due to already start");
  }
}
void NonBlockHttpClient::killTaskRequest(){
  if(isStartInfoInit){
    LongPollingTaskComInfo tmp={
      .type=KILL_TASK,
      .ptr=nullptr,
    };
    xQueueSend(
      startInfo.motherToChild,
      &tmp,
      portMAX_DELAY
    );
    status=NBC_WAIT_KILL;
    Serial.println("kill queue is sended");
  }
}

void NonBlockHttpClient::check(){
  if(!isStartInfoInit){
    return;
  }
  LongPollingTaskComInfo comInfo;
  const BaseType_t ret=xQueueReceive(
    startInfo.childToMother,
    &comInfo,
    0
  );
  if(ret==pdTRUE){
    switch(comInfo.type){
      case RECEIVE_DATA:
      setNewReceiveData(static_cast<String*>(comInfo.ptr));
      Serial.println(receiveData->c_str());
      status=NBC_NOCONNECT;
      break;

      case FAIL_DATA:
      setNewReceiveData(nullptr);
      Serial.println("receive data failed");
      status=NBC_NOCONNECT;
      break;

      case DONE_KILL_TASK:
      Serial.println("done task");
      status=NBC_INIT;
      isStartInfoInit=false;
      vQueueDelete(startInfo.childToMother);
      vQueueDelete(startInfo.motherToChild);
      Serial.println("done task clear ok");
      break;
    }
  }
}
void NonBlockHttpClient::postRequest(const String &payload,const uint32_t timeout_ms){
  auto *info=new PostGetNowInfo;
  info->payloadStringP=new String(payload);
  info->timeout=timeout_ms;
  LongPollingTaskComInfo tmp={
    .type=POST_NOW,
    .ptr=static_cast<void*>(info),
  };
  xQueueSend(
    startInfo.motherToChild,
    &tmp,
    portMAX_DELAY
  );
  status=NBC_CONNECT;
}
void NonBlockHttpClient::getRequest(const uint32_t timeout_ms){
  auto *info=new PostGetNowInfo;
  info->payloadStringP=nullptr;
  info->timeout=timeout_ms;
  LongPollingTaskComInfo tmp={
    .type=GET_NOW,
    .ptr=static_cast<void*>(info),
  };
  xQueueSend(
    startInfo.motherToChild,
    &tmp,
    portMAX_DELAY
  );
  status=NBC_CONNECT;
}
String *NonBlockHttpClient::refReceiveDataP(){
  return receiveData;
}
uint32_t NonBlockHttpClient::getReceiveDataVersion(){
  return receiveDataVersion;
}

NonBlockHttpClient::Status NonBlockHttpClient::getStatus(){
  return status;
}

SingleLongPollingAgent::SingleLongPollingAgent():
  lpp(nullptr),targetStatusVersion(0),currentInternalVersion(0)
{
}
void SingleLongPollingAgent::setSingleLongPollingP(NonBlockHttpClient *lppArg){
  lpp=lppArg;
}
void SingleLongPollingAgent::sendRequest(const uint32_t timeout_ms,const uint32_t version){
  Serial.printf("send request with timeout:%d version:%d\n",timeout_ms,version);
  String payload;
  ArduinoJson::StaticJsonDocument<100> doc;
  doc["timeout"]=timeout_ms;
  doc["version"]=version;
  ArduinoJson::serializeJson(doc,payload);
  lpp->postRequest(payload,timeout_ms);

  targetStatusVersion=version;
}
bool SingleLongPollingAgent::checkExistNewReceiveData(){
  if(currentInternalVersion!=lpp->getReceiveDataVersion()){
    currentInternalVersion=lpp->getReceiveDataVersion();
    return true;
  }
  return false;
}
bool SingleLongPollingAgent::isNoConnect(){
  return lpp->getStatus()==NonBlockHttpClient::NBC_NOCONNECT;
}
bool SingleLongPollingAgent::isStop(){
  return lpp->getStatus()==NonBlockHttpClient::NBC_INIT;
}
uint32_t SingleLongPollingAgent::getTargetStatusVersion(){
  return targetStatusVersion;
}

void SingleLongPollingTotal::start(const String urlArg){
  main.startTask(urlArg);
  agent.setSingleLongPollingP(&main);
}
void SingleLongPollingTotal::check(){
  main.check();
}
NonBlockHttpClient *SingleLongPollingTotal::refMainP(){
  return &main;
}
SingleLongPollingAgent *SingleLongPollingTotal::refAgentP(){
  return &agent;
}

}