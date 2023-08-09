#include "mfrc.hpp"

namespace mfrc{

MfrcHelper::MfrcHelper(MFRC522 *mfrc522PArg):
mfrc522P(mfrc522PArg){
}

char MfrcHelper::conv4bitToHexChar(uint8_t val){
  if(val<10){
    return '0'+val;
  }else if(val<16){
    return 'A'+val-10;
  }else{
    return 'X';
  }
}

bool MfrcHelper::isReadyNewCard(){
  if(mfrc522P->PICC_IsNewCardPresent()){
    if(mfrc522P->PICC_ReadCardSerial()){
      return true;
    }
  }
  return false;
}

char * MfrcHelper::writeToUidCharArrayP(char hexTmp[]){
  uint8_t strIndex=0;
  for(byte i=0;i<mfrc522P->uid.size;i++){
    const byte target=mfrc522P->uid.uidByte[i];
    hexTmp[strIndex++]=conv4bitToHexChar(target>>4);
    hexTmp[strIndex++]=conv4bitToHexChar(target&15);
  }
  hexTmp[strIndex]='\0';
  return hexTmp;
}

const String &MfrcHelper::refUidString(){
  char hexTmp[21];
  stringBuf.clear();
  writeToUidCharArrayP(hexTmp);
  stringBuf.concat(hexTmp);
  return stringBuf;
}

}//namespace mfrc