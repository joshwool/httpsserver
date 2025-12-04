#ifndef SERVER_H
#define SERVER_H


#include "threadpool.h"

#ifdef __linux__
#include <sys/epoll.h>
#endif
#ifdef __APPLE__ || __FreeBSD__
#include <sys/event.h>
#endif

class Server {
  public:
  Server(const char *port, unsigned int threads = std::thread::hardware_concurrency());
  ~Server();
  void run();

  private:
  ThreadPool threadpool;
  int socketfd;
  std::map<int, std::shared_ptr<connection>> connections;
  SSL_CTX *ctx;
};


#endif // SERVER_H
