#include "../headers/connection.h"

connection::~connection() {
  map->erase(fd);
  SSL_shutdown(ssl);
  SSL_free(ssl);
}