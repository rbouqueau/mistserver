#include "output_http.h"
#include <lib/ts_packet.h>
#include <lib/mp4.h>
#include <lib/mp4_generic.h>

namespace Mist {
  class OutHDS : public HTTPOutput {
    public:
      OutHDS(Socket::Connection & conn);
      ~OutHDS();
      static void init(Util::Config * cfg);
      void onHTTP();
      void sendNext();
    protected:
      void getTracks();
      std::string dynamicBootstrap(int tid);
      std::string dynamicIndex();
      std::set<int> videoTracks;///<< Holds valid video tracks for playback
      long long int audioTrack;///<< Holds audio track ID for playback
      long long unsigned int playUntil;
      FLV::Tag tag;
  };
}

typedef Mist::OutHDS mistOut;
