#include "../headers/connection.h"

#include <iostream>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <unistd.h>

#define BUFSIZE 4096

void respond(const std::shared_ptr<connection> &conn) {
  if (!conn->handshaken) {
    if (SSL_accept(conn->ssl) <= 0) {
      std::cerr << "thread: TLS handshake failed" << std::endl;
      ERR_print_errors_fp(stderr);
      exit(1);
    }
    conn->handshaken = true;
  } else {
    char buf[BUFSIZE];
    int status = SSL_read(conn->ssl, buf, BUFSIZE);
    if (status == -1) {
      std::cerr << "thread: read failed" << std::endl;
      ERR_print_errors_fp(stderr);
      exit(1);
    } else if (status == 0) {
      SSL_shutdown(conn->ssl);
    } else {

    }
  }
}
