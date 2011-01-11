#include "box_stts.h"

Box_stts::Box_stts( ) {
  Container = new Box( 0x73747473 );
  SetReserved();
}

Box_stts::~Box_stsc() {
  delete Container;
}

Box * Box_stts::GetBox() {
  return Container;
}

void Box_stts::SetReserved( ) {
  Container->SetPayload((uint32_t)4,Box::uint32_to_uint8(0));
}

void Box_stts::AddEntry( uint32_t SampleCount, uint32_t SamplesDelta, uint32_t Offset = 0 ) {
  if(Offset >= Entries.size()) {
    Entries.resize(Offset+1);
  }
  Entries[Offset].SampleCount = SampleCount;
  Entries[Offset].SampleDelta = SampleDelta;
  WriteEntries( );
}


void Box_stts::WriteEntries( ) {
  Container->ResetPayload();
  SetReserved( );
  if(!Offsets.empty()) {
    for(int32_t i = Offsets.size() -1; i >= 0; i--) {
      Container->SetPayload((uint32_t)4,Box::uint32_to_uint8(Entries[i].SampleDelta),(i*8)+12);
      Container->SetPayload((uint32_t)4,Box::uint32_to_uint8(Entries[i].SampleCount),(i*8)+8);
    }
  }
  Container->SetPayload((uint32_t)4,Box::uint32_to_uint8(Offsets.size()),4);
}