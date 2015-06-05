#include <lib/socket.h>
#include <lib/json.h>

namespace Controller {
  void checkConfig(JSON::Value & in, JSON::Value & out);
  bool authorize(JSON::Value & Request, JSON::Value & Response, Socket::Connection & conn);
  int handleAPIConnection(Socket::Connection & conn);
}
