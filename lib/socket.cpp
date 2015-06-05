/// \file socket.cpp
/// A handy Socket wrapper library.
/// Written by Jaron Vietor in 2010 for DDVTech

#include "socket.h"
#include "timing.h"
#include "defines.h"
#include <sys/stat.h>
#ifndef _WIN32
#include <netdb.h>
#endif
#include <algorithm>
#include <sstream>
#include <cstdlib>

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

#define BUFFER_BLOCKSIZE 4096 //set buffer blocksize to 4KiB

#if defined(__CYGWIN__) || defined(_WIN32)
#include <ws2tcpip.h>
#ifndef EWOULDBLOCK
  #define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef EINTR
  #define EINTR WSAEINTR
#endif
#define SOCKETSIZE 8092ul
static void kill_WSA(void){
  WSACleanup();
}

static void initWSA(){
  static bool wsa_inited = false;
  if (!wsa_inited){
    WSAData wsaData;
    if (!WSAStartup(MAKEWORD(1, 1), &wsaData)){
      atexit(kill_WSA);
    }
  }
}
#else
#define SOCKETSIZE 51200ul
#endif

std::string uint2string(unsigned int i) {
  std::stringstream st;
  st << i;
  return st.str();
}

/// Returns the amount of elements in the internal std::deque of std::string objects.
/// The back is popped as long as it is empty, first - this way this function is
/// guaranteed to return 0 if the buffer is empty.
unsigned int Socket::Buffer::size() {
  while (data.size() > 0 && data.back().empty()) {
    data.pop_back();
  }
  return data.size();
}

/// Returns either the amount of total bytes available in the buffer or max, whichever is smaller.
unsigned int Socket::Buffer::bytes(unsigned int max) {
  unsigned int i = 0;
  for (std::deque<std::string>::iterator it = data.begin(); it != data.end(); ++it) {
    i += (*it).size();
    if (i >= max) {
      return max;
    }
  }
  return i;
}

/// Appends this string to the internal std::deque of std::string objects.
/// It is automatically split every BUFFER_BLOCKSIZE bytes.
void Socket::Buffer::append(const std::string & newdata) {
  append(newdata.c_str(), newdata.size());
}

/// Appends this data block to the internal std::deque of std::string objects.
/// It is automatically split every BUFFER_BLOCKSIZE bytes.
void Socket::Buffer::append(const char * newdata, const unsigned int newdatasize) {
  unsigned int i = 0, j = 0;
  while (i < newdatasize) {
    j = i;
    while (j < newdatasize && j - i <= BUFFER_BLOCKSIZE) {
      j++;
      if (newdata[j - 1] == '\n') {
        break;
      }
    }
    if (i != j) {
      data.push_front(std::string(newdata + i, (size_t)(j - i)));
      i = j;
    } else {
      break;
    }
  }
  if (data.size() > 5000) {
    DEBUG_MSG(DLVL_WARN, "Warning: After %d new bytes, buffer has %d parts!", newdatasize, (int)data.size());
  }
}

/// Prepends this data block to the internal std::deque of std::string objects.
/// It is _not_ automatically split every BUFFER_BLOCKSIZE bytes.
void Socket::Buffer::prepend(const std::string & newdata) {
  data.push_back(newdata);
}

/// Prepends this data block to the internal std::deque of std::string objects.
/// It is _not_ automatically split every BUFFER_BLOCKSIZE bytes.
void Socket::Buffer::prepend(const char * newdata, const unsigned int newdatasize) {
  data.push_back(std::string(newdata, (size_t)newdatasize));
}

/// Returns true if at least count bytes are available in this buffer.
bool Socket::Buffer::available(unsigned int count) {
  unsigned int i = 0;
  for (std::deque<std::string>::iterator it = data.begin(); it != data.end(); ++it) {
    i += (*it).size();
    if (i >= count) {
      return true;
    }
  }
  return false;
}

/// Removes count bytes from the buffer, returning them by value.
/// Returns an empty string if not all count bytes are available.
std::string Socket::Buffer::remove(unsigned int count) {
  if (!available(count)) {
    return "";
  }
  unsigned int i = 0;
  std::string ret;
  ret.reserve(count);
  for (std::deque<std::string>::reverse_iterator it = data.rbegin(); it != data.rend(); ++it) {
    if (i + (*it).size() < count) {
      ret.append(*it);
      i += (*it).size();
      (*it).clear();
    } else {
      ret.append(*it, 0, count - i);
      (*it).erase(0, count - i);
      break;
    }
  }
  return ret;
}

/// Copies count bytes from the buffer, returning them by value.
/// Returns an empty string if not all count bytes are available.
std::string Socket::Buffer::copy(unsigned int count) {
  if (!available(count)) {
    return "";
  }
  unsigned int i = 0;
  std::string ret;
  ret.reserve(count);
  for (std::deque<std::string>::reverse_iterator it = data.rbegin(); it != data.rend(); ++it) {
    if (i + (*it).size() < count) {
      ret.append(*it);
      i += (*it).size();
    } else {
      ret.append(*it, 0, count - i);
      break;
    }
  }
  return ret;
}

/// Gets a reference to the back of the internal std::deque of std::string objects.
std::string & Socket::Buffer::get() {
  static std::string empty;
  if (data.size() > 0) {
    return data.back();
  } else {
    return empty;
  }
}

/// Completely empties the buffer
void Socket::Buffer::clear() {
  data.clear();
}

/// Create a new base socket. This is a basic constructor for converting any valid socket to a Socket::Connection.
/// \param sockNo Integer representing the socket to convert.
Socket::Connection::Connection(int sockNo) {
  sock = sockNo;
  pipes[0] = -1;
  pipes[1] = -1;
  up = 0;
  down = 0;
  conntime = Util::epoch();
  Error = false;
  Blocking = true;
} //Socket::Connection basic constructor

/// Simulate a socket using two file descriptors.
/// \param write The filedescriptor to write to.
/// \param read The filedescriptor to read from.
Socket::Connection::Connection(int write, int read) {
  sock = -1;
  pipes[0] = write;
  pipes[1] = read;
  up = 0;
  down = 0;
  conntime = Util::epoch();
  Error = false;
  Blocking = true;
} //Socket::Connection basic constructor

/// Create a new disconnected base socket. This is a basic constructor for placeholder purposes.
/// A socket created like this is always disconnected and should/could be overwritten at some point.
Socket::Connection::Connection() {
  sock = -1;
  pipes[0] = -1;
  pipes[1] = -1;
  up = 0;
  down = 0;
  conntime = Util::epoch();
  Error = false;
  Blocking = true;
} //Socket::Connection basic constructor

/// Internally used call to make an file descriptor blocking or not.
void setFDBlocking(int FD, bool blocking) {
  #ifdef _WIN32
  u_long newState = blocking?0:1;
  ioctlsocket(FD, FIONBIO, &newState);
  #else
  int flags = fcntl(FD, F_GETFL, 0);
  if (!blocking) {
    flags |= O_NONBLOCK;
  } else {
    flags &= !O_NONBLOCK;
  }
  fcntl(FD, F_SETFL, flags);
  #endif
}

/// Internally used call to make an file descriptor blocking or not.
bool isFDBlocking(int FD) {
  #ifdef _WIN32
  return true;
  #else
  int flags = fcntl(FD, F_GETFL, 0);
  return !(flags & O_NONBLOCK);
  #endif
}

/// Set this socket to be blocking (true) or nonblocking (false).
void Socket::Connection::setBlocking(bool blocking) {
  Blocking = blocking;
  if (sock >= 0) {
    setFDBlocking(sock, blocking);
  }
  if (pipes[0] >= 0) {
    setFDBlocking(pipes[0], blocking);
  }
  if (pipes[1] >= 0) {
    setFDBlocking(pipes[1], blocking);
  }
}

/// Set this socket to be blocking (true) or nonblocking (false).
bool Socket::Connection::isBlocking(){
#ifdef _WIN32
  return Blocking;
#else
  if (sock >= 0) {
    return isFDBlocking(sock);
  }
  if (pipes[0] >= 0) {
    return isFDBlocking(pipes[0]);
  }
  if (pipes[1] >= 0) {
    return isFDBlocking(pipes[1]);
  }
  return false;
#endif
}

/// Close connection. The internal socket is closed and then set to -1.
/// If the connection is already closed, nothing happens.
/// This function calls shutdown, thus making the socket unusable in all other
/// processes as well. Do not use on shared sockets that are still in use.
void Socket::Connection::close() {
  if (sock != -1) {
    #ifdef _WIN32
    shutdown(sock, 2);
    #else
    shutdown(sock, SHUT_RDWR);
    #endif
  }
  drop();
} //Socket::Connection::close

/// Close connection. The internal socket is closed and then set to -1.
/// If the connection is already closed, nothing happens.
/// This function does *not* call shutdown, allowing continued use in other
/// processes.
void Socket::Connection::drop() {
  if (connected()) {
    #ifdef _WIN32
    if (sock != -1){closesocket(sock); sock = -1;}
    if (pipes[0] != -1){closesocket(pipes[0]); pipes[0] = -1;}
    if (pipes[1] != -1){closesocket(pipes[1]); pipes[1] = -1;}
    #else
    if (sock != -1) {
      DEBUG_MSG(DLVL_HIGH, "Socket %d closed", sock);
      errno = EINTR;
      while (::close(sock) != 0 && errno == EINTR) {
      }
      sock = -1;
    }
    if (pipes[0] != -1) {
      errno = EINTR;
      while (::close(pipes[0]) != 0 && errno == EINTR) {
      }
      pipes[0] = -1;
    }
    if (pipes[1] != -1) {
      errno = EINTR;
      while (::close(pipes[1]) != 0 && errno == EINTR) {
      }
      pipes[1] = -1;
    }
    #endif
  }
} //Socket::Connection::drop

/// Returns internal socket number.
int Socket::Connection::getSocket() {
  if (sock != -1){
    return sock;
  }
  if (pipes[0] != -1) {
    return pipes[0];
  }
  if (pipes[1] != -1) {
    return pipes[1];
  }
  return -1;
}

/// Returns non-piped internal socket number.
int Socket::Connection::getPureSocket() {
  return sock;
}

/// Returns a string describing the last error that occured.
/// Only reports errors if an error actually occured - returns the host address or empty string otherwise.
std::string Socket::Connection::getError() {
  return remotehost;
}

/// Create a new TCP Socket. This socket will (try to) connect to the given host/port right away.
/// \param host String containing the hostname to connect to.
/// \param port String containing the port to connect to.
/// \param nonblock Whether the socket should be nonblocking.
Socket::Connection::Connection(std::string host, int port, bool nonblock) {
  #ifdef _WIN32
  initWSA();
  #endif
  pipes[0] = -1;
  pipes[1] = -1;
  struct addrinfo * result, *rp, hints;
  Error = false;
  Blocking = true;
  up = 0;
  down = 0;
  conntime = Util::epoch();
  std::stringstream ss;
  ss << port;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  #ifndef _WIN32
  hints.ai_flags = AI_ADDRCONFIG;
  #endif
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  int s = getaddrinfo(host.c_str(), ss.str().c_str(), &hints, &result);
  if (s != 0) {
    DEBUG_MSG(DLVL_FAIL, "Could not connect to %s:%i! Error: %s", host.c_str(), port, gai_strerror(s));
    close();
    return;
  }

  remotehost = "";
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock < 0) {
      continue;
    }
    if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
      break;
    }
    remotehost += strerror(errno);
#ifndef _MSC_VER
    ::close(sock);
#else
	closesocket(sock);
#endif
  }
  freeaddrinfo(result);

  if (rp == 0) {
    DEBUG_MSG(DLVL_FAIL, "Could not connect to %s! Error: %s", host.c_str(), remotehost.c_str());
    close();
  } else {
    if (nonblock) {
      setBlocking(false);
    }
  }
} //Socket::Connection TCP Contructor

/// Returns the connected-state for this socket.
/// Note that this function might be slightly behind the real situation.
/// The connection status is updated after every read/write attempt, when errors occur
/// and when the socket is closed manually.
/// \returns True if socket is connected, false otherwise.
bool Socket::Connection::connected() const {
  return (sock >= 0) || ((pipes[0] >= 0) && (pipes[1] >= 0));
}

/// Returns the time this socket has been connected.
unsigned int Socket::Connection::connTime() {
	return (unsigned)conntime;
}

/// Returns total amount of bytes sent.
unsigned int Socket::Connection::dataUp() {
  return up;
}

/// Returns total amount of bytes received.
unsigned int Socket::Connection::dataDown() {
  return down;
}

/// Returns a std::string of stats, ended by a newline.
/// Requires the current connector name as an argument.
std::string Socket::Connection::getStats(std::string C) {
  return "S " + getHost() + " " + C + " " + uint2string((unsigned)(Util::epoch() - conntime)) + " " + uint2string(up) + " " + uint2string(down) + "\n";
}

/// Updates the downbuffer internal variable.
/// Returns true if new data was received, false otherwise.
bool Socket::Connection::spool() {
  /// \todo Provide better mechanism to prevent overbuffering.
  if (downbuffer.size() > 10000) {
    return true;
  } else {
    return iread(downbuffer);
  }
}

bool Socket::Connection::peek() {
  /// clear buffer
  downbuffer.clear();
  return iread(downbuffer, MSG_PEEK);
}

/// Returns a reference to the download buffer.
Socket::Buffer & Socket::Connection::Received() {
  return downbuffer;
}

/// Will not buffer anything but always send right away. Blocks.
/// Any data that could not be send will block until it can be send or the connection is severed.
void Socket::Connection::SendNow(const char * data, size_t len) {
  bool bing = isBlocking();
  if (!bing) {
    setBlocking(true);
  }
  unsigned int i = iwrite(data, std::min((long unsigned int)len, SOCKETSIZE));
  while (i < len && connected()) {
    i += iwrite(data + i, std::min((long unsigned int)(len - i), SOCKETSIZE));
  }
  if (!bing) {
    setBlocking(false);
  }
}

/// Will not buffer anything but always send right away. Blocks.
/// Any data that could not be send will block until it can be send or the connection is severed.
void Socket::Connection::SendNow(const char * data) {
  int len = strlen(data);
  SendNow(data, len);
}

/// Will not buffer anything but always send right away. Blocks.
/// Any data that could not be send will block until it can be send or the connection is severed.
void Socket::Connection::SendNow(const std::string & data) {
  SendNow(data.data(), data.size());
}

/// Incremental write call. This function tries to write len bytes to the socket from the buffer,
/// returning the amount of bytes it actually wrote.
/// \param buffer Location of the buffer to write from.
/// \param len Amount of bytes to write.
/// \returns The amount of bytes actually written.
unsigned int Socket::Connection::iwrite(const void * buffer, int len) {
  if (!connected() || len < 1) {
    return 0;
  }
  int r;
  #ifdef _WIN32
  if (sock >= 0) {
    r = send(sock, (const char*)buffer, len, 0);
  } else {
    r = send(pipes[0], (const char*)buffer, len, 0);
  }
  #else
  if (sock >= 0) {
    r = send(sock, buffer, len, 0);
  } else {
    r = write(pipes[0], buffer, len);
  }
  #endif
  if (r < 0) {
    switch (errno) {
      case EWOULDBLOCK:
        return 0;
        break;
      default:
        if (errno != EPIPE && errno != ECONNRESET) {
          Error = true;
          remotehost = strerror(errno);
          DEBUG_MSG(DLVL_WARN, "Could not iwrite data! Error: %s", remotehost.c_str());
        }
        close();
        return 0;
        break;
    }
  }
  if (r == 0 && (sock >= 0)) {
    close();
  }
  up += r;
  return r;
} //Socket::Connection::iwrite

/// Incremental read call. This function tries to read len bytes to the buffer from the socket,
/// returning the amount of bytes it actually read.
/// \param buffer Location of the buffer to read to.
/// \param len Amount of bytes to read.
/// \param flags Flags to use in the recv call. Ignored on fake sockets.
/// \returns The amount of bytes actually read.
int Socket::Connection::iread(void * buffer, int len, int flags) {
  if (!connected() || len < 1) {
    return 0;
  }
  int r;
  #ifdef _WIN32
  if (sock >= 0) {
    r = recv(sock, (char*)buffer, len, flags);
  } else {
    r = recv(pipes[1], (char*)buffer, len, flags);
  }
  #else
  if (sock >= 0) {
    r = recv(sock, buffer, len, flags);
  } else {
    r = recv(pipes[1], buffer, len, flags);
    if (r < 0 && errno == ENOTSOCK){
      r = read(pipes[1], buffer, len);
    }
  }
  #endif
  if (r < 0) {
    #ifdef _WIN32
    int currErr = WSAGetLastError();
    #else
    int currErr = errno;
    #endif
    switch (currErr) {
      case EWOULDBLOCK:
        return 0;
        break;
      case EINTR:
        return 0;
        break;
      default:
        if (errno != EPIPE) {
          Error = true;
          remotehost = strerror(errno);
          DEBUG_MSG(DLVL_WARN, "Could not iread data! Error: %s", remotehost.c_str());
        }
        close();
        return 0;
        break;
    }
  }
  if (r == 0) {
    close();
  }
  down += r;
  return r;
} //Socket::Connection::iread

/// Read call that is compatible with Socket::Buffer.
/// Data is read using iread (which is nonblocking if the Socket::Connection itself is),
/// then appended to end of buffer.
/// \param buffer Socket::Buffer to append data to.
/// \param flags Flags to use in the recv call. Ignored on fake sockets.
/// \return True if new data arrived, false otherwise.
bool Socket::Connection::iread(Buffer & buffer, int flags) {
  char cbuffer[BUFFER_BLOCKSIZE];
  int num = iread(cbuffer, BUFFER_BLOCKSIZE, flags);
  if (num < 1) {
    return false;
  }
  buffer.append(cbuffer, num);
  return true;
} //iread

/// Incremental write call that is compatible with std::string.
/// Data is written using iwrite (which is nonblocking if the Socket::Connection itself is),
/// then removed from front of buffer.
/// \param buffer std::string to remove data from.
/// \return True if more data was sent, false otherwise.
bool Socket::Connection::iwrite(std::string & buffer) {
  if (buffer.size() < 1) {
    return false;
  }
  unsigned int tmp = iwrite((void *)buffer.c_str(), buffer.size());
  if (!tmp) {
    return false;
  }
  buffer = buffer.substr(tmp);
  return true;
} //iwrite

/// Gets hostname for connection, if available.
std::string Socket::Connection::getHost() {
  return remotehost;
}

/// Gets hostname for connection, if available.
/// Guaranteed to be either empty or 16 bytes long.
std::string Socket::Connection::getBinHost() {
  if (remotehost.size()){
    struct addrinfo * result, *rp, hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    #ifndef _WIN32
    hints.ai_flags = AI_ADDRCONFIG;
    #endif
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    int s = getaddrinfo(remotehost.c_str(), 0, &hints, &result);
    if (s != 0) {
      DEBUG_MSG(DLVL_FAIL, "Could not resolve '%s'! Error: %s", remotehost.c_str(), gai_strerror(s));
      return "";
    }
    char tmpBuffer[17] = "\000\000\000\000\000\000\000\000\000\000\377\377\000\000\000\000";
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      if (rp->ai_family == AF_INET){
        memcpy(tmpBuffer + 12, &((sockaddr_in *)rp->ai_addr)->sin_addr.s_addr, 4);
      }
      if (rp->ai_family == AF_INET6){
        memcpy(tmpBuffer, ((sockaddr_in6 *)rp->ai_addr)->sin6_addr.s6_addr, 16);
      }
    }
    freeaddrinfo(result);
    return std::string(tmpBuffer, 16);
  }else{
    return "";
  }
}

/// Sets hostname for connection manually.
/// Overwrites the detected host, thus possibily making it incorrect.
void Socket::Connection::setHost(std::string host) {
  remotehost = host;
}

/// Returns true if these sockets are the same socket.
/// Does not check the internal stats - only the socket itself.
bool Socket::Connection::operator==(const Connection & B) const {
  return sock == B.sock && pipes[0] == B.pipes[0] && pipes[1] == B.pipes[1];
}

/// Returns true if these sockets are not the same socket.
/// Does not check the internal stats - only the socket itself.
bool Socket::Connection::operator!=(const Connection & B) const {
  return sock != B.sock || pipes[0] != B.pipes[0] || pipes[1] != B.pipes[1];
}

/// Returns true if the socket is valid.
/// Aliases for Socket::Connection::connected()
Socket::Connection::operator bool() const {
  return connected();
}

/// Returns true if the given address can be matched with the remote host.
/// Can no longer return true after any socket error have occurred.
bool Socket::Connection::isAddress(std::string addr) {
  struct addrinfo * result, *rp, hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  #ifndef _WIN32
  hints.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED;
  #endif
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  int s = getaddrinfo(addr.c_str(), 0, &hints, &result);
  if (s != 0) {
    return false;
  }

  char newaddr[INET_ADDRSTRLEN];
  newaddr[0] = 0;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    #ifndef _WIN32
    /// \todo Make this work for windows. Haha. Yeah, right.
    if (rp->ai_family == AF_INET && inet_ntop(rp->ai_family, &(((sockaddr_in *)rp->ai_addr)->sin_addr), newaddr, INET_ADDRSTRLEN)) {
      DEBUG_MSG(DLVL_DEVEL, "Comparing: '%s'  to '%s'", remotehost.c_str(), newaddr);
      if (remotehost == newaddr) {
        return true;
      }
      DEBUG_MSG(DLVL_DEVEL, "Comparing: '%s'  to '::ffff:%s'", remotehost.c_str(), newaddr);
      if (remotehost == std::string("::ffff:") + newaddr) {
        return true;
      }
    }
    if (rp->ai_family == AF_INET6 && inet_ntop(rp->ai_family, &(((sockaddr_in6 *)rp->ai_addr)->sin6_addr), newaddr, INET_ADDRSTRLEN)) {
      DEBUG_MSG(DLVL_DEVEL, "Comparing: '%s'  to '%s'", remotehost.c_str(), newaddr);
      if (remotehost == newaddr) {
        return true;
      }
    }
    #endif
  }
  freeaddrinfo(result);
  return false;
}

/// Create a new base Server. The socket is never connected, and a placeholder for later connections.
Socket::Server::Server() {
  sock = -1;
} //Socket::Server base Constructor

/// Create a new TCP Server. The socket is immediately bound and set to listen.
/// A maximum of 100 connections will be accepted between accept() calls.
/// Any further connections coming in will be dropped.
/// \param port The TCP port to listen on
/// \param hostname (optional) The interface to bind to. The default is 0.0.0.0 (all interfaces).
/// \param nonblock (optional) Whether accept() calls will be nonblocking. Default is false (blocking).
Socket::Server::Server(int port, std::string hostname, bool nonblock) {
  #ifdef _WIN32
  initWSA();
  #endif
  if (!IPv6bind(port, hostname, nonblock) && !IPv4bind(port, hostname, nonblock)) {
    DEBUG_MSG(DLVL_FAIL, "Could not create socket %s:%i! Error: %s", hostname.c_str(), port, errors.c_str());
    sock = -1;
  }
} //Socket::Server TCP Constructor

/// Attempt to bind an IPv6 socket.
/// \param port The TCP port to listen on
/// \param hostname The interface to bind to. The default is 0.0.0.0 (all interfaces).
/// \param nonblock Whether accept() calls will be nonblocking. Default is false (blocking).
/// \return True if successful, false otherwise.
bool Socket::Server::IPv6bind(int port, std::string hostname, bool nonblock) {
  sock = socket(AF_INET6, SOCK_STREAM, 0);
  if (sock < 0) {
    errors = strerror(errno);
    DEBUG_MSG(DLVL_ERROR, "Could not create IPv6 socket %s:%i! Error: %s", hostname.c_str(), port, errors.c_str());
    return false;
  }
  int on = 1;
  #ifdef _WIN32
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
  on = 0;
  setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&on, sizeof(on));
  #else
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  #endif
  if (nonblock) {
    setBlocking(!nonblock);
  }
  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(port); //set port
  #ifdef _WIN32
  addr.sin6_addr = in6addr_any;
  #else
  if (hostname == "0.0.0.0" || hostname.length() == 0) {
    addr.sin6_addr = in6addr_any;
  } else {
    inet_pton(AF_INET6, hostname.c_str(), &addr.sin6_addr); //set interface, 0.0.0.0 (default) is all
  }
  #endif
  int ret = bind(sock, (sockaddr *) &addr, sizeof(addr)); //do the actual bind
  if (ret == 0) {
    ret = listen(sock, 100); //start listening, backlog of 100 allowed
    if (ret == 0) {
      DEBUG_MSG(DLVL_DEVEL, "IPv6 socket success @ %s:%i", hostname.c_str(), port);
      return true;
    } else {
      errors = strerror(errno);
      DEBUG_MSG(DLVL_ERROR, "IPv6 listen failed! Error: %s", errors.c_str());
      close();
      return false;
    }
  } else {
    errors = strerror(errno);
    DEBUG_MSG(DLVL_ERROR, "IPv6 Binding %s:%i failed (%s)", hostname.c_str(), port, errors.c_str());
    close();
    return false;
  }
}

/// Attempt to bind an IPv4 socket.
/// \param port The TCP port to listen on
/// \param hostname The interface to bind to. The default is 0.0.0.0 (all interfaces).
/// \param nonblock Whether accept() calls will be nonblocking. Default is false (blocking).
/// \return True if successful, false otherwise.
bool Socket::Server::IPv4bind(int port, std::string hostname, bool nonblock) {
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    errors = strerror(errno);
    DEBUG_MSG(DLVL_ERROR, "Could not create IPv4 socket %s:%i! Error: %s", hostname.c_str(), port, errors.c_str());
    return false;
  }
  int on = 1;
  #ifdef _WIN32
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
  #else
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  #endif
  if (nonblock) {
    setBlocking(!nonblock);
  }
  struct sockaddr_in addr4;
  memset(&addr4, 0, sizeof(addr4));
  addr4.sin_family = AF_INET;
  addr4.sin_port = htons(port); //set port
  #ifdef _WIN32
  addr4.sin_addr.s_addr = INADDR_ANY;
  #else
  if (hostname == "0.0.0.0" || hostname.length() == 0) {
    addr4.sin_addr.s_addr = INADDR_ANY;
  } else {
    inet_pton(AF_INET, hostname.c_str(), &addr4.sin_addr); //set interface, 0.0.0.0 (default) is all
  }
  #endif
  int ret = bind(sock, (sockaddr *) &addr4, sizeof(addr4)); //do the actual bind
  if (ret == 0) {
    ret = listen(sock, 100); //start listening, backlog of 100 allowed
    if (ret == 0) {
      DEBUG_MSG(DLVL_DEVEL, "IPv4 socket success @ %s:%i", hostname.c_str(), port);
      return true;
    } else {
      errors = strerror(errno);
      DEBUG_MSG(DLVL_ERROR, "IPv4 listen failed! Error: %s", errors.c_str());
      close();
      return false;
    }
  } else {
    errors = strerror(errno);
    DEBUG_MSG(DLVL_ERROR, "IPv4 Binding %s:%i failed (%s)", hostname.c_str(), port, errors.c_str());
    close();
    return false;
  }
}

/// Accept any waiting connections. If the Socket::Server is blocking, this function will block until there is an incoming connection.
/// If the Socket::Server is nonblocking, it might return a Socket::Connection that is not connected, so check for this.
/// \param nonblock (optional) Whether the newly connected socket should be nonblocking. Default is false (blocking).
/// \returns A Socket::Connection, which may or may not be connected, depending on settings and circumstances.
Socket::Connection Socket::Server::accept(bool nonblock) {
  if (sock < 0) {
    return Socket::Connection(-1);
  }
  struct sockaddr_in6 addrinfo;
  socklen_t len = sizeof(addrinfo);
  static char addrconv[INET6_ADDRSTRLEN];
  int r = ::accept(sock, (sockaddr *) &addrinfo, &len);
  //set the socket to be nonblocking, if requested.
  //we could do this through accept4 with a flag, but that call is non-standard...
  Socket::Connection tmp(r);
  if (r < 0) {
    if ((errno != EWOULDBLOCK) && (errno != EAGAIN) && (errno != EINTR)) {
      DEBUG_MSG(DLVL_FAIL, "Error during accept - closing server socket %d.", sock);
      close();
    }
  } else {
    if (nonblock){
      tmp.setBlocking(!nonblock);
    }
    #ifndef _WIN32
    if (addrinfo.sin6_family == AF_INET6) {
      tmp.remotehost = inet_ntop(AF_INET6, &(addrinfo.sin6_addr), addrconv, INET6_ADDRSTRLEN);
      DEBUG_MSG(DLVL_HIGH, "IPv6 addr [%s]", tmp.remotehost.c_str());
    }
    if (addrinfo.sin6_family == AF_INET) {
      tmp.remotehost = inet_ntop(AF_INET, &(((sockaddr_in *) &addrinfo)->sin_addr), addrconv, INET6_ADDRSTRLEN);
      DEBUG_MSG(DLVL_HIGH, "IPv4 addr [%s]", tmp.remotehost.c_str());
    }
    #endif
  }
  return tmp;
}

/// Set this socket to be blocking (true) or nonblocking (false).
void Socket::Server::setBlocking(bool blocking) {
  if (sock >= 0) {
    setFDBlocking(sock, blocking);
  }
}

/// Set this socket to be blocking (true) or nonblocking (false).
bool Socket::Server::isBlocking() {
  if (sock >= 0) {
    return isFDBlocking(sock);
  }
  return false;
}

/// Close connection. The internal socket is closed and then set to -1.
/// If the connection is already closed, nothing happens.
/// This function calls shutdown, thus making the socket unusable in all other
/// processes as well. Do not use on shared sockets that are still in use.
void Socket::Server::close() {
  if (sock != -1) {
    #ifdef _WIN32
    shutdown(sock, 2);
    #else
    shutdown(sock, SHUT_RDWR);
    #endif
  }
  drop();
} //Socket::Server::close

/// Close connection. The internal socket is closed and then set to -1.
/// If the connection is already closed, nothing happens.
/// This function does *not* call shutdown, allowing continued use in other
/// processes.
void Socket::Server::drop() {
  if (connected()) {
    if (sock != -1) {
      #ifdef _WIN32
      closesocket(sock);
      #else
      DEBUG_MSG(DLVL_HIGH, "ServerSocket %d closed", sock);
      errno = EINTR;
      while (::close(sock) != 0 && errno == EINTR) {
      }
      #endif
      sock = -1;
    }
  }
} //Socket::Server::drop

/// Returns the connected-state for this socket.
/// Note that this function might be slightly behind the real situation.
/// The connection status is updated after every accept attempt, when errors occur
/// and when the socket is closed manually.
/// \returns True if socket is connected, false otherwise.
bool Socket::Server::connected() const {
  return (sock >= 0);
} //Socket::Server::connected

/// Returns internal socket number.
int Socket::Server::getSocket() {
  return sock;
}

/// Create a new UDP Socket.
/// Will attempt to create an IPv6 UDP socket, on fail try a IPV4 UDP socket.
/// If both fail, prints an DLVL_FAIL debug message.
/// \param nonblock Whether the socket should be nonblocking.
Socket::UDPConnection::UDPConnection(bool nonblock) {
  sock = socket(AF_INET6, SOCK_DGRAM, 0);
  if (sock == -1) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
  }
  if (sock == -1) {
    DEBUG_MSG(DLVL_FAIL, "Could not create UDP socket: %s", strerror(errno));
  }
  up = 0;
  down = 0;
  destAddr = 0;
  destAddr_size = 0;
  data = 0;
  data_size = 0;
  data_len = 0;
  if (nonblock) {
    setBlocking(!nonblock);
  }
} //Socket::UDPConnection UDP Contructor

/// Copies a UDP socket, re-allocating local copies of any needed structures.
/// The data/data_size/data_len variables are *not* copied over.
Socket::UDPConnection::UDPConnection(const UDPConnection & o) {
  sock = socket(AF_INET6, SOCK_DGRAM, 0);
  if (sock == -1) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
  }
  if (sock == -1) {
    DEBUG_MSG(DLVL_FAIL, "Could not create UDP socket: %s", strerror(errno));
  }
  up = 0;
  down = 0;
  if (o.destAddr && o.destAddr_size) {
    destAddr = malloc(o.destAddr_size);
    if (destAddr) {
      memcpy(destAddr, o.destAddr, o.destAddr_size);
    }
  } else {
    destAddr = 0;
    destAddr_size = 0;
  }
  data = 0;
  data_size = 0;
  data_len = 0;
}

/// Closes the UDP socket, cleans up any memory allocated by the socket.
Socket::UDPConnection::~UDPConnection() {
  if (sock != -1) {
    errno = EINTR;
    while (
#ifndef _MSC_VER
		::close(sock)
#else
		closesocket(sock)
#endif		
		!= 0 && errno == EINTR) {
    }
    sock = -1;
  }
  if (destAddr) {
    free(destAddr);
    destAddr = 0;
  }
  if (data) {
    free(data);
    data = 0;
  }
}

/// Stores the properties of the receiving end of this UDP socket.
/// This will be the receiving end for all SendNow calls.
void Socket::UDPConnection::SetDestination(std::string destIp, uint32_t port) {
  if (destAddr) {
    free(destAddr);
    destAddr = 0;
  }
  destAddr = malloc(sizeof(struct sockaddr_in6));
  if (destAddr) {
    destAddr_size = sizeof(struct sockaddr_in6);
    memset(destAddr, 0, destAddr_size);
    ((struct sockaddr_in6 *)destAddr)->sin6_family = AF_INET6;
    ((struct sockaddr_in6 *)destAddr)->sin6_port = htons(port);
    #ifndef _WIN32
    if (inet_pton(AF_INET6, destIp.c_str(), &(((struct sockaddr_in6 *)destAddr)->sin6_addr)) == 1) {
      return;
    }
    #endif
    memset(destAddr, 0, destAddr_size);
    ((struct sockaddr_in *)destAddr)->sin_family = AF_INET;
    ((struct sockaddr_in *)destAddr)->sin_port = htons(port);
    #ifndef _WIN32
    if (inet_pton(AF_INET, destIp.c_str(), &(((struct sockaddr_in *)destAddr)->sin_addr)) == 1) {
      return;
    }
    #endif
  }
  free(destAddr);
  destAddr = 0;
  DEBUG_MSG(DLVL_FAIL, "Could not set destination for UDP socket: %s:%d", destIp.c_str(), port);
}//Socket::UDPConnection SetDestination

/// Gets the properties of the receiving end of this UDP socket.
/// This will be the receiving end for all SendNow calls.
void Socket::UDPConnection::GetDestination(std::string & destIp, uint32_t & port) {
  if (!destAddr || !destAddr_size) {
    destIp = "";
    port = 0;
    return;
  }
  char addr_str[INET6_ADDRSTRLEN + 1];
  addr_str[INET6_ADDRSTRLEN] = 0;//set last byte to zero, to prevent walking out of the array
  if (((struct sockaddr_in *)destAddr)->sin_family == AF_INET6) {
    #ifndef _WIN32
    if (inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)destAddr)->sin6_addr), addr_str, INET6_ADDRSTRLEN) != 0) {
      destIp = addr_str;
      port = ntohs(((struct sockaddr_in6 *)destAddr)->sin6_port);
      return;
    }
    #endif
  }
  if (((struct sockaddr_in *)destAddr)->sin_family == AF_INET) {
    #ifndef _WIN32
    if (inet_ntop(AF_INET, &(((struct sockaddr_in *)destAddr)->sin_addr), addr_str, INET6_ADDRSTRLEN) != 0) {
      destIp = addr_str;
      port = ntohs(((struct sockaddr_in *)destAddr)->sin_port);
      return;
    }
    #endif
  }
  destIp = "";
  port = 0;
  DEBUG_MSG(DLVL_FAIL, "Could not get destination for UDP socket");
}//Socket::UDPConnection GetDestination

/// Sets the socket to be blocking if the parameters is true.
/// Sets the socket to be non-blocking otherwise.
void Socket::UDPConnection::setBlocking(bool blocking) {
  if (sock >= 0) {
    setFDBlocking(sock, blocking);
  }
}

/// Sends a UDP datagram using the buffer sdata.
/// This function simply calls SendNow(const char*, size_t)
void Socket::UDPConnection::SendNow(const std::string & sdata) {
  SendNow(sdata.c_str(), sdata.size());
}

/// Sends a UDP datagram using the buffer sdata.
/// sdata is required to be NULL-terminated.
/// This function simply calls SendNow(const char*, size_t)
void Socket::UDPConnection::SendNow(const char * sdata) {
  int len = strlen(sdata);
  SendNow(sdata, len);
}

/// Sends a UDP datagram using the buffer sdata of length len.
/// Does not do anything if len < 1.
/// Prints an DLVL_FAIL level debug message if sending failed.
void Socket::UDPConnection::SendNow(const char * sdata, size_t len) {
  if (len < 1) {
    return;
  }
  int r = sendto(sock, sdata, len, 0, (sockaddr *)destAddr, destAddr_size);
  if (r > 0) {
    up += r;
  } else {
    DEBUG_MSG(DLVL_FAIL, "Could not send UDP data through %d: %s", sock, strerror(errno));
  }
}

/// Bind to a port number, returning the bound port.
/// Attempts to bind over IPv6 first.
/// If it fails, attempts to bind over IPv4.
/// If that fails too, gives up and returns zero.
/// Prints a debug message at DLVL_FAIL level if binding failed.
/// \return Actually bound port number, or zero on error.
int Socket::UDPConnection::bind(int port) {
  struct sockaddr_in6 s6;
  s6.sin6_family = AF_INET6;
  s6.sin6_addr = in6addr_any;
  if (port) {
    s6.sin6_port = htons(port);
  }
  int r = ::bind(sock, (sockaddr *)&s6, sizeof(s6));
  if (r == 0) {
    return ntohs(s6.sin6_port);
  }

  struct sockaddr_in s4;
  s4.sin_family = AF_INET;
  s4.sin_addr.s_addr = INADDR_ANY;
  if (port) {
    s4.sin_port = htons(port);
  }
  r = ::bind(sock, (sockaddr *)&s4, sizeof(s4));
  if (r == 0) {
    return ntohs(s4.sin_port);
  }

  DEBUG_MSG(DLVL_FAIL, "Could not bind UDP socket to port %d", port);
  return 0;
}

/// Attempt to receive a UDP packet.
/// This will automatically allocate or resize the internal data buffer if needed.
/// If a packet is received, it will be placed in the "data" member, with it's length in "data_len".
/// \return True if a packet was received, false otherwise.
bool Socket::UDPConnection::Receive() {
  #ifdef _WIN32
  int r = recvfrom(sock, data, data_size, MSG_PEEK, 0, 0);
  #else
  int r = recvfrom(sock, data, data_size, MSG_PEEK | MSG_TRUNC, 0, 0);
  #endif
  if (data_size < (unsigned int)r) {
    data = (char *)realloc(data, r);
    if (data) {
      data_size = r;
    } else {
      data_size = 0;
    }
  }
  socklen_t destsize = destAddr_size;
  r = recvfrom(sock, data, data_size, 0, (sockaddr *)destAddr, &destsize);
  if (r > 0) {
    down += r;
    data_len = r;
    return true;
  } else {
    data_len = 0;
    return false;
  }
}

int Socket::UDPConnection::getSock() {
  return sock;
}
