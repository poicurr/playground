#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

template <class F, class... Args>
using ResultType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

template <class F, class... Args>
auto makeTask(F &&f, Args &&...args)
    -> std::shared_ptr<std::packaged_task<ResultType<F, Args...>()>> {
  using TaskType = std::packaged_task<ResultType<F, Args...>()>;
  return std::make_shared<TaskType>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
}

class ThreadPool;

class Worker {
public:
  explicit Worker(ThreadPool &pool) : m_pool(pool) {}
  ~Worker() = default;
  void operator()();

private:
  ThreadPool &m_pool;
};

class ThreadPool {
public:
  explicit ThreadPool(std::size_t nThreads) : m_stop(false) {
    for (std::size_t i = 0; i < nThreads; ++i) {
      m_workers.emplace_back(Worker(*this));
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lock(m_queueMutex);
      m_stop = true;
    }
    m_condition.notify_all();
    for (auto &worker : m_workers) {
      worker.join();
    }
  }

  template <typename F, typename... Args>
  auto enqueue(F &&f, Args &&...args) -> std::future<ResultType<F, Args...>> {
    auto task = makeTask(std::forward<F>(f), std::forward<Args>(args)...);

    {
      std::lock_guard<std::mutex> lock(m_queueMutex);
      if (m_stop) {
        throw std::runtime_error("enqueue on stopped ThreadPool");
      }
      m_tasks.emplace_back([task]() { (*task)(); });
    }
    m_condition.notify_one();
    return task->get_future();
  }

private:
  friend class Worker;

  std::vector<std::thread> m_workers;
  std::deque<std::function<void()>> m_tasks;

  std::mutex m_queueMutex;
  std::condition_variable m_condition;
  bool m_stop;
};

inline void Worker::operator()() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(m_pool.m_queueMutex);
      m_pool.m_condition.wait(
          lock, [this]() { return m_pool.m_stop || !m_pool.m_tasks.empty(); });

      if (m_pool.m_stop && m_pool.m_tasks.empty()) {
        return;
      }

      task = std::move(m_pool.m_tasks.front());
      m_pool.m_tasks.pop_front();
    }
    task();
  }
}
