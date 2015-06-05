#include "output_http.h"
#include <lib/http_parser.h>

namespace Mist {
  class OutHSS : public HTTPOutput {
    public:
      OutHSS(Socket::Connection & conn);
      ~OutHSS();
      static void init(Util::Config * cfg);
      void onHTTP();
      void sendNext();
      void initialize();/*LTS*/
      void sendHeader();
    protected:
      JSON::Value encryption;
      std::string smoothIndex();
      int canSeekms(unsigned int ms);
      int keysToSend;
      int myTrackStor;
      int myKeyStor;
      unsigned long long playUntil;
  };
}

typedef Mist::OutHSS mistOut;
