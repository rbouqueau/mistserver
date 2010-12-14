#include "box.h"

Box::Box() {
  Payload = NULL;
  PayloadSize = 0;
}

Box::Box(uint32_t BoxType) {
  header.BoxType = BoxType;
  Payload = NULL;
  PayloadSize = 0;
}

Box::~Box() {
}

void Box::SetBoxType(uint32_t BoxType) {
  header.BoxType = BoxType;
}

uint32_t Box::GetBoxType() {
  return header.BoxType;
}

void Box::SetPayload(uint32_t Size, uint8_t * Data, uint32_t Index) {
  uint8_t * tempchar = NULL;
  if ( Index + Size > PayloadSize ) {
    if ( Payload ) {
      tempchar = new uint8_t[PayloadSize];
      memcpy( tempchar, Payload, PayloadSize );
      delete Payload;
    }
    PayloadSize = Index + Size;
    Payload = new uint8_t[PayloadSize];
    if( tempchar ) {
      memcpy( Payload, tempchar, Index );
    } else {
      for(uint32_t i = 0; i < Index; i++) { Payload[i] = 0; }
    }
    memcpy( &Payload[Index], Data, Size );
    header.TotalSize = PayloadSize + 8;
    if( tempchar ) {
      delete tempchar;
    }
  } else {
    memcpy( &Payload[Index], Data, Size );
  }
}

uint32_t Box::GetPayloadSize() {
  return PayloadSize;
}

uint8_t * Box::GetPayload() {
  uint8_t * temp = new uint8_t[PayloadSize];
  memcpy( temp, Payload, PayloadSize );
  return temp;
}

uint8_t * Box::GetPayload(uint32_t Index, uint32_t & Size) {
  if(Index > PayloadSize) { return NULL; }
  if(Index + Size > PayloadSize) { Size = PayloadSize - Index; }
  uint8_t * temp = new uint8_t[Size - Index];
  memcpy( temp, &Payload[Index], Size - Index );
  return temp;
}

uint8_t * Box::uint32_to_uint8( uint32_t data ) {
  uint8_t * temp = new uint8_t[4];
  temp[0] = (data >> 24) & 0x000000FF;
  temp[1] = (data >> 16 ) & 0x000000FF;
  temp[2] = (data >> 8 ) & 0x000000FF;
  temp[3] = (data ) & 0x000000FF;
  return temp;
}