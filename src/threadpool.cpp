#include "../headers/threadpool.h"
#include "respond.cpp"

ThreadPool::ThreadPool(unsigned int threadNum): terminate(false) {
  for (unsigned int i = 0; i < threadNum; i++) {
    threads.emplace_back(&ThreadPool::loop, this);
  }
}

void ThreadPool::loop() {
  while (true) {
    std::shared_ptr<connection> conn;
    {
      std::unique_lock lock(queue_mutex);
      condition.wait(lock, [this] {
        return !conns.empty() || terminate;
      });
      if (terminate) {
        return;
      }
      conn = conns.front();
      conns.pop();
    }
    respond(conn);
  }
}

void ThreadPool::queueTask(const std::shared_ptr<connection> conn) {
  {
    std::unique_lock lock(queue_mutex);
    conns.push(conn);
  }
  condition.notify_one();
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock lock(queue_mutex);
    terminate = true;
  }
  condition.notify_all();
  for (auto &thread : threads) {
    thread.join();
  }
  threads.clear();
}