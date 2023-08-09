#ifndef MFRC_HPP_INCLUDE_GUARD
#define MFRC_HPP_INCLUDE_GUARD

#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>

namespace mfrc{


class MfrcHelper{
private:
  MFRC522 * const mfrc522P;
  char conv4bitToHexChar(uint8_t);
  String stringBuf;
public:
  MfrcHelper(MFRC522 *mfrc522PArg);
  bool isReadyNewCard();
  
  // 必ずlen=21以上のバッファーを渡すこと
  char *writeToUidCharArrayP(char[]);

  const String &refUidString();
};

}//namespace mfrc

#endif