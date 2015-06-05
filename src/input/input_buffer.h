#include "input.h"
#include <lib/dtsc.h>
#include <lib/shared_memory.h>

namespace Mist {
  class inputBuffer : public Input {
    public:
      inputBuffer(Util::Config * cfg);
      ~inputBuffer();
    private:
      unsigned int bufferTime;
      unsigned int cutTime;
    protected:
      //Private Functions
      bool setup();
      void updateMeta();
      bool readHeader();
      void getNext(bool smart = true);
      void updateTrackMeta(unsigned long tNum);
      void updateMetaFromPage(unsigned long tNum, unsigned long pageNum);
      void seek(int seekTime);
      void trackSelect(std::string trackSpec); 
      bool removeKey(unsigned int tid);
      void removeUnused();
      void finish();
      void userCallback(char * data, size_t len, unsigned int id);
      std::set<unsigned long> negotiatingTracks;
      std::set<unsigned long> activeTracks;
      std::map<unsigned long, unsigned long long> lastUpdated;
      ///Maps trackid to a pagenum->pageData map
      std::map<unsigned long, std::map<unsigned long, DTSCPageData> > bufferLocations;
      std::map<unsigned long, char *> pushLocation;
      inputBuffer * singleton;
  };
}

typedef Mist::inputBuffer mistIn;


