#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#endif

#include INPUTTYPE 
#include <lib/config.h>
#include <lib/defines.h>

int main(int argc, char * argv[]) {
  Util::Config conf(argv[0], PACKAGE_VERSION);
  mistIn conv(&conf);
  if (conf.parseArgs(argc, argv)) {
    IPC::semaphore playerLock;
    if(conf.getString("streamname").size()){
      playerLock.open(std::string("/lock_" + conf.getString("streamname")).c_str(), O_CREAT | O_RDWR, ACCESSPERMS, 1);
      if (!playerLock.tryWait()){
        DEBUG_MSG(DLVL_DEVEL, "A player for stream %s is already running", conf.getString("streamname").c_str());
        return 1;
      }
    }
    conf.activate();
    #ifndef _WIN32
    while (conf.is_active){
      int pid = fork();
      if (pid == 0){
        playerLock.close();
        return conv.run();
      }
      if (pid == -1){
        DEBUG_MSG(DLVL_FAIL, "Unable to spawn player process");
        playerLock.post();
        return 2;
      }
      //wait for the process to exit
      int status;
      while (waitpid(pid, &status, 0) != pid && errno == EINTR) continue;
      //if the exit was clean, don't restart it
      if (WIFEXITED(status) && (WEXITSTATUS(status) == 0)){
        DEBUG_MSG(DLVL_MEDIUM, "Finished player succesfully");
        break;
      }
      if (DEBUG >= DLVL_DEVEL){
        DEBUG_MSG(DLVL_DEVEL, "Player exited with errors - stopping because this is a development build.");
        break;
      }
    }
    #else
    conv.run();
    #endif
    playerLock.post();
    playerLock.close();
  }
  return 0;
}

