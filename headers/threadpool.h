#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "connection.h"

#include <queue>
#include <thread>

class ThreadPool {
  public:
  ThreadPool();
  ThreadPool(unsigned int threads);

  void loop();
  void queueTask(const std::shared_ptr<connection> conn);

  ~ThreadPool();

  private:
  bool terminate;
  std::vector<std::thread> threads;
  std::queue<std::shared_ptr<connection>> conns;
  std::mutex queue_mutex;
  std::condition_variable condition;
};

#endif //THREADPOOL_H
