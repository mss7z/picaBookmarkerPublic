#ifndef INTERNET_INTERNAL_HPP_INCLUDE_GUARD
#define INTERNET_INTERNAL_HPP_INCLUDE_GUARD

#include <Arduino.h>
#include <ArduinoJson.hpp>
#include <HTTPClient_mod.h>

namespace internet{

class RegularC_ms{
private:
	unsigned long interval;
	unsigned long lastTime;
public:
	RegularC_ms(unsigned long intervalArg,unsigned long start=0):
	interval(intervalArg)
	{
		lastTime=start-interval;
	}
	bool ist(){
		if(interval<(unsigned long)(millis()-lastTime)){
			lastTime=millis();
			return true;
		}else{
			return false;
		}
	}
	void set(unsigned long val){interval=val;}
	unsigned long read(){return interval;}
	operator bool(){return ist();}
};

struct LongPollingTaskStartInfo{
  String url;
  QueueHandle_t childToMother;
  QueueHandle_t motherToChild;
};
enum TaskTalkType{
  POST_NOW,
  GET_NOW,
  RECEIVE_DATA,
  FAIL_DATA,
  FAIL_BEGIN,
  KILL_TASK,
  DONE_KILL_TASK,
};
struct PostGetNowInfo{
  String *payloadStringP;
  uint32_t timeout;
};
struct LongPollingTaskComInfo{
  TaskTalkType type;
  void *ptr;
};

void longPollingTask(void *longPollingTaskStartInfo);
void longPollingTaskCore(void *longPollingTaskStartInfo);

class NonBlockHttpClient{
public:
  enum Status{
    NBC_INIT,
    NBC_NOCONNECT,
    NBC_CONNECT,
    NBC_WAIT_KILL
  };
private:
  LongPollingTaskStartInfo startInfo;
  bool isStartInfoInit;
  TaskHandle_t task;
  String *receiveData=nullptr;
  //GASとの通信で使われるVersionとは別物なので注意が必要
  uint32_t receiveDataVersion=0;
  Status status=NBC_INIT;

  void setNewReceiveData(String *);
public:
  NonBlockHttpClient();
  void startTask(const String urlArg);
  void killTaskRequest();
  void check();
  void postRequest(const String &,const uint32_t timeout_ms);
  void getRequest(const uint32_t timeout_ms);
  String *refReceiveDataP();
  uint32_t getReceiveDataVersion();
  Status getStatus();
};
class SingleLongPollingAgent{
private:
  // lpp:=LongPollingPtr
  NonBlockHttpClient *lpp;
  uint32_t targetStatusVersion;
  uint32_t currentInternalVersion;
public:
  SingleLongPollingAgent();
  void setSingleLongPollingP(NonBlockHttpClient *lppArg);
  void sendRequest(const uint32_t timeout_ms,const uint32_t version);
  bool checkExistNewReceiveData();
  bool isNoConnect();
  bool isStop();
  uint32_t getTargetStatusVersion();
};
class SingleLongPollingTotal{
private:
  NonBlockHttpClient main;
  SingleLongPollingAgent agent;
public:
  void start(const String urlArg);
  void check();
  NonBlockHttpClient *refMainP();
  SingleLongPollingAgent *refAgentP();
};

}//namespace internet

#endif