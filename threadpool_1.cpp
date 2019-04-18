#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <iostream>

class ThreadPool {
public:
    ThreadPool(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();
private:
    // 线程池
    std::vector< std::thread > workers;
    // 任务队列
    std::queue< std::function<void()> > tasks;

    // synchronization同步
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
        :   stop(false)
{
    //创建n个线程，每个线程等待是否有新的task, 或者线程stop（要终止）
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
                [this]
                {
                    for(;;)//轮询
                    {
                        std::function<void()> task;

                        {
                        //获取同步锁
                                                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                                                    //线程会一直阻塞，直到有新的task，或者是线程要退出
                            this->condition.wait(lock,
                                                 [this]{ return this->stop || !this->tasks.empty(); });
                                                 //线程退出
                            if(this->stop && this->tasks.empty())
                                return;
                                //将task取出
                            task = std::move(this->tasks.front());
                            //队列中移除以及取出的task
                            this->tasks.pop();
                        }
                        //执行task,完了则进入下一次循环
                        task();
                    }
                }
        );
}

// add new work item to the pool
// 将队列压入线程池,其中f是要执行的函数， args是多有的参数
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
    //返回的结果的类型，当然可以根据实际的需要去掉这个(gcc4.7的c++11不支持)
    using return_type = typename std::result_of<F(Args...)>::type;
//将函数handle与参数绑定
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    //after finishing the task, then get result by res.get() (mainly used in the invoked function)
    std::future<return_type> res = task->get_future();
    {
        //压入队列需要线程安全，因此需要先获取锁
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");
        //任务压入队列
        tasks.emplace([task](){ (*task)(); });
    }
    //添加了新的task，因此条件变量通知其他线程可以获取task执行
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();//通知所有在等待锁的线程
    //等待所有的线程任务执行完成退出
    for(std::thread &worker: workers)
        worker.join();
}



//模拟下载，sleep 2S
void dummy_download(std::string url){
  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::cout<<" complete download: "<<url<<std::endl;
}

//根据id返回用户名
std::string get_user_name(int id){
  std::this_thread::sleep_for(std::chrono::seconds(2));
  return "user_" + std::to_string(id);
}


int main()
{
  ThreadPool tp(5);

  //线程执行结果的返回
  std::vector<std::future<std::string>> vecStr;
  // 下载对应的url链接，没有返回值
  tp.enqueue(dummy_download, "www.baidu.jpg");
  tp.enqueue(dummy_download, "www.yy.jpg");

  //数据库根据id查询user name
  vecStr.emplace_back(tp.enqueue(get_user_name, 101));
  vecStr.emplace_back(tp.enqueue(get_user_name, 102));

  //输出线程返回的值，实际中可以不要
  std::future<void> res1 = std::async(std::launch::async, [&vecStr](){
    for (auto &&ret:vecStr) {
      std::cout<<"get user: "<<ret.get()<<std::endl;
    }
    std::cout<<std::endl;
  });
  return 0;
}







