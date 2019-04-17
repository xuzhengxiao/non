#include <condition_variable>
#include <future>
#include <deque>
#include <thread>
#include <iostream>
#include <vector>

class ThreadPool {
 public:
  ThreadPool(size_t _threads);

  /// Stops and waits for the worker thread to exit, flushing all
  /// pending lambdas
  ~ThreadPool();

  /// Request that the worker thread stop itself
  void stop();

  /// Blocking waits in the current thread for the worker thread to
  /// stop
  void waitForThreadExit();

  /// Adds a lambda to run on the worker thread; returns a future that
  /// can be used to block on its completion.
  /// Future status is `true` if the lambda was run in the worker
  /// thread; `false` if it was not run, because the worker thread is
  /// exiting or has exited.
  std::future<bool> add(std::function<void()> f);

 private:
  void startThread();
  void threadMain();
  void threadLoop();
  
  size_t threads;
  /// Thread that all queued lambdas are run on
  //std::thread thread_;
  std::vector<std::thread> workers;
  /// Mutex for the queue and exit status
  std::mutex mutex_;

  /// Monitor for the exit status and the queue
  std::condition_variable monitor_;

  /// Whether or not we want the thread to exit
  bool wantStop_;

  /// Queue of pending lambdas to call
  std::deque<std::pair<std::function<void()>, std::promise<bool>>> queue_;
};

ThreadPool::ThreadPool(size_t _threads) : threads(_threads),
    wantStop_(false) {
  startThread();

  // Make sure that the thread has started before continuing
  add([](){}).get();
}

ThreadPool::~ThreadPool() {
  stop();
  waitForThreadExit();
}

void
ThreadPool::startThread() {
 for(size_t i=0;i<threads;i++)
   workers.emplace_back(std::thread([this](){ threadMain(); }));
}

void
ThreadPool::stop() {
  std::lock_guard<std::mutex> guard(mutex_);

  wantStop_ = true;
  monitor_.notify_all();
}

std::future<bool>
ThreadPool::add(std::function<void()> f) {
  std::lock_guard<std::mutex> guard(mutex_);

  if (wantStop_) {
    // The timer thread has been stopped, or we want to stop; we can't
    // schedule anything else
    std::promise<bool> p;
    auto fut = p.get_future();

    // did not execute
    p.set_value(false);
    return fut;
  }

  auto pr = std::promise<bool>();
  auto fut = pr.get_future();

  queue_.emplace_back(std::make_pair(std::move(f), std::move(pr)));

  // Wake up our thread
  monitor_.notify_one();
  return fut;
}

void
ThreadPool::threadMain() {
  threadLoop();

  // Call all pending tasks
  //FAISS_ASSERT(wantStop_);
  /*
  for (auto& f : queue_) {
    f.first();
    f.second.set_value(true);
  }*/
}

void
ThreadPool::threadLoop() {
  while (true) {
    std::pair<std::function<void()>, std::promise<bool>> data;

    {
      std::unique_lock<std::mutex> lock(mutex_);

      while (!wantStop_ && queue_.empty()) {
        monitor_.wait(lock);
      }

      if (wantStop_ && queue_.empty()) {
        return;
      }

      data = std::move(queue_.front());
      queue_.pop_front();
    }

    data.first();
    data.second.set_value(true);
  }
}

void
ThreadPool::waitForThreadExit() {
  try {
    for(std::thread &t:workers)
      t.join();
  } catch (...) {
  }
}

void t1()
{
  std::cout<<"hello t1"<<std::endl;
}
void t2()
{
  std::cout<<"hello t2"<<std::endl;
}
void t3()
{
  std::cout<<"hello t3"<<std::endl;
}
void t4()
{
  std::cout<<"hello t4"<<std::endl;
}

int main()
{
  ThreadPool trial(2);
  trial.add(t1);
  trial.add(t2);
  trial.stop();
  trial.add(t3);
  trial.add(t4);
}



