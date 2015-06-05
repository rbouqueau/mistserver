#include "input.h"
#include <lib/dtsc.h>

namespace Mist {
  class inputDTSC : public Input {
    public:
      inputDTSC(Util::Config * cfg);
    protected:
      //Private Functions
      bool setup();
      bool readHeader();
      void getNext(bool smart = true);
      void seek(int seekTime);
      void trackSelect(std::string trackSpec);

      DTSC::File inFile;
  };
}

typedef Mist::inputDTSC mistIn;


