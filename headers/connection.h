#ifndef CONNECTION_H
#define CONNECTION_H

#include <map>
#include <memory>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

struct connection {
  int fd;
  bool handshaken;
  SSL *ssl;
  sockaddr conninfo;
  std::map<int, std::shared_ptr<connection>> *map;
  ~connection();
};

#endif //CONNECTION_H
