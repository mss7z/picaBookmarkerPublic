#ifndef TAPE_LED_HPP_INCLUDE_GUARD
#define TAPE_LED_HPP_INCLUDE_GUARD

#include <Arduino.h>
#include <FastLED.h>

namespace tapeLed{

class TapeLedHelper{
private:
  CRGB* leds;
  const size_t LEDS_LEN;
public:
  TapeLedHelper(CRGB *ledsArg,const size_t ledsLenArg):
    leds{ledsArg},LEDS_LEN{ledsLenArg}
  {
  }
  void test(){
    leds[0] = CRGB::Red; 
    FastLED.show();
  }
  void clearMemory(){
    for(size_t i=0;i<LEDS_LEN;i++){
      leds[i]=CRGB::Black;
    }
  }
  void onMemory(const size_t ledNum,const CRGB color){
    leds[ledNum]=color;
  }
  void show(){
    FastLED.show();
  }
};

}//namespace tapeLed

#endif