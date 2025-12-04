#include "../headers/server.h"

// NETWORKING LIBS
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>

// MISCELLANEOUS LIBS
#include <iostream>

#define MAX_EVENTS 10
#define BACKLOG 10

inline void setNonBlocking(const int &fd) {
  if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
    std::cerr << "failed to set socket option O_NONBLOCK" << std::endl;
    exit(1);
  }
}


Server::Server(const char *port, unsigned int threads) : threadpool(threads), socketfd(-1) {
  const SSL_METHOD *method = TLS_server_method();

  if (!(ctx = SSL_CTX_new(method))) {
    std::cerr << "failed to create SSL context" << std::endl;
    ERR_print_errors_fp(stderr);
  }

  if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 ) {
    ERR_print_errors_fp(stderr);
    exit(1);
  }


  addrinfo hints = {}, *results, *servaddr;

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (int status; (status = getaddrinfo(nullptr, port, &hints, &results) != 0)) {
    std::cerr << "error - getaddrinfo: " << gai_strerror(status) << std::endl;
  }

  for (servaddr = results; servaddr; servaddr = servaddr->ai_next) {
    if ((socketfd = socket(servaddr->ai_family, servaddr->ai_socktype, servaddr->ai_protocol)) == -1) {
      std::cerr << "error - socket: " << strerror(errno) << std::endl;
      continue;
    }

    int opt = 1;

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1) {
      std::cerr << "failed to set socket option, SO_REUSEADDR" << std::endl;
      exit(1);
    }

    if (bind(socketfd, servaddr->ai_addr, servaddr->ai_addrlen) == -1) {
      close(socketfd);
      socketfd = -1;
      perror("server: bind");
      continue;
    }
    break;
  }

  freeaddrinfo(results);

  if (servaddr == nullptr) {
    std::cerr << "server: failed to bind" << std::endl;
    exit(1);
  }
}

Server::~Server() {
  if (socketfd != -1) {
    close(socketfd);
  }
  SSL_CTX_free(ctx);
}

void Server::run() {
  if (listen(socketfd, BACKLOG) == -1) {
    std::cerr << "server: failed to listen" << std::endl;
    exit(1);
  }

#ifdef __linux__
  int epollfd, nfds;
  struct epoll_event event, events[MAX_EVENTS];

  if ((epollfd = epoll_create1(0)) == -1) {
    std::cerr << "server: failed to create epoll" << std::endl;
    exit(1);
  }

  event.events = EPOLLIN;
  event.data.fd = socketfd;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event) == -1) {
    std::cerr << "server: failed to add epoll event" << std::endl;
  }
#endif
#ifdef __APPLE__ || __FreeBSD__
  int kqueue = -1, newev, fd, connfd;
  struct kevent changelist, events[MAX_EVENTS];
  struct sockaddr conninfo;

  if ((kqueue = kqueue()) == -1) {
    std::cerr << "server: failed to create kqueue" << std::endl;
    exit(1);
  }

  EV_SET(&changelist, socketfd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
  if (kevent(kqueue, &changelist, 1, nullptr, 0, nullptr) == -1) {
    std::cerr << "server: failed to change kqueue" << std::endl;
    close(kqueue);
    exit(1);
  }
#endif

  while (true) {
#ifdef __linux__
    if ((nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1)) == -1) {
      std::cerr << "server: failed to poll" << std::endl;
      exit(1);
    }

    for (n = 0; n < nfds; ++n) {
      if (events[n].data.fd == socketfd) {
#endif
#ifdef __APPLE__ || __FreeBSD__
        if ((newev = kevent(kqueue, nullptr, 0, events, MAX_EVENTS, nullptr)) == -1) {
          std::cerr << "server: failed to create new event" << std::endl;
          close(kqueue);
          exit(1);
        }
        for (int i = 0; i < newev; i++) {
          fd = static_cast<int>(events[i].ident);
#endif
          if (fd == socketfd) {
            if ((connfd = accept(socketfd, &conninfo, nullptr)) == -1) {
              std::cerr << "server: failed to accept connection" << std::endl;
              continue;
            }
            SSL *ssl = SSL_new(ctx);
            SSL_set_fd(ssl, connfd);

            std::shared_ptr<connection> conn(new connection{connfd, false,  ssl, conninfo, &connections});
            connections.emplace(fd, std::move(conn));
          } else {
            threadpool.queueTask(connections.at(fd));
          }
        }
      }
    }
