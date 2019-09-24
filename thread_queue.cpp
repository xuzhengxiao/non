#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
using namespace std;


template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue() {}
    threadsafe_queue(threadsafe_queue const& other)
    {
        std::lock_guard<std::mutex> lk(other.mut);
        data_queue = other.data_queue;
    }
    void push(T new_value)//入队操作
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(new_value);
        //cout<<new_value<<" has been put"<<endl;
        data_cond.notify_one();
    }
    void wait_and_pop(T& value)//直到有元素可以删除为止
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] {return !data_queue.empty(); });
        value = data_queue.front();
        //cout<<value<<" has been popped"<<endl;
        data_queue.pop();
    }
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] {return !data_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }
    bool try_pop(T& value)//不管有没有队首元素直接返回
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = data_queue.front();
        data_queue.pop();
        return true;
    }
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        cout<<"judge is empty?"<<endl;
        return data_queue.empty();
    }
};


void push(int id,threadsafe_queue<int>& q)
{
    for(int i=0;i<2;i++)
    {
        q.push(id*2+i);
        cout<<"id "<<id<<" push "<<id*5+i<<endl;
    }
}

void pop(int id,threadsafe_queue<int>& q)
{
    int v;
    for(int i=0;i<3;i++)
    {
        q.wait_and_pop(v);
        cout<<"id "<<id<<" pop "<<v<<endl;
    }
}

int main()
{
    threadsafe_queue<int> q;
//    int v;
//    q.push(3);
//    q.wait_and_pop(v);
    thread p1(push,0,std::ref(q));
    thread p2(push,1,std::ref(q));
    thread p3(push,2,std::ref(q));
    thread c1(pop,3,std::ref(q));
    thread c2(pop,4,std::ref(q));
    p1.join();
    p2.join();
    p3.join();
    c1.join();
    c2.join();
    return 0;
}
