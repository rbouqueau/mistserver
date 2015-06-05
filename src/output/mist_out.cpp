#include OUTPUTTYPE
#include <lib/config.h>
#include <lib/socket.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#define getpid _getpid
#endif

int spawnForked(Socket::Connection & S){
  mistOut tmp(S);
  return tmp.run();
}

#ifdef NOT_MAIN
int output(int argc, char * argv[]) {
#else
int main(int argc, char * argv[]) {
#endif
  Util::Config conf(argv[0], PACKAGE_VERSION);
  mistOut::init(&conf);
  if (conf.parseArgs(argc, argv)) {
    if (conf.getBool("json")) {
      std::cout << mistOut::capa.toString() << std::endl;
      return -1;
    }
    if (mistOut::listenMode()){
      conf.serveForkedSocket(spawnForked);
    }else{
      Socket::Connection S(fileno(stdout),fileno(stdin) );
      mistOut tmp(S);
      return tmp.run();
    }
  }
  return 0;
}
