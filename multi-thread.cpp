#include <iostream>                // std::cout
#include <thread>                // std::thread, std::this_thread::yield
#include <mutex>                // std::mutex, std::unique_lock
#include <condition_variable>    // std::condition_variable

/*
  当前线程调用 wait() 后将被阻塞(此时当前线程应该获得了锁（mutex），不妨设获得锁 lck)，直到另外某个线程调用 notify_* 唤醒了当前线程。

  在线程被阻塞时，该函数会自动调用 lck.unlock() 释放锁，使得其他被阻塞在锁竞争上的线程得以继续执行。
  另外，一旦当前线程获得通知(notified，通常是另外某个线程调用 notify_* 唤醒了当前线程)，wait() 函数也是自动调用 lck.lock()，使得 lck 的状态和 wait 函数被调用时相同。

  std::unique_lock对象以独占所有权的方式(unique owership)管理mutex对象的上锁和解锁操作，
  即在unique_lock对象的声明周期内，它所管理的锁对象会一直保持上锁状态；而unique_lock的生命周期结束之后，它所管理的锁对象会被解锁
*/

struct BoundedBuffer {
     int* buffer;
     int capacity;
 
     int front;
     int rear;
     int count;
 
     std::mutex lock;
 
     std::condition_variable not_full;
     std::condition_variable not_empty;
 
     BoundedBuffer(int capacity) : capacity(capacity), front(0), rear(0), count(0) {
         buffer = new int[capacity];
     }
     ~BoundedBuffer(){
         delete[] buffer;
     }
     void deposit(int data){
         std::unique_lock<std::mutex> l(lock);
         not_full.wait(l, [this](){return count != capacity; });
         buffer[rear] = data;
         rear = (rear + 1) % capacity;
         ++count;
         not_empty.notify_one();
     }
     int fetch(){
         std::unique_lock<std::mutex> l(lock);
         not_empty.wait(l, [this](){return count != 0; });
         int result = buffer[front];
         front = (front + 1) % capacity;
         --count;
         not_full.notify_one();
         return result;
     }
 };
 
 
 void consumer(int id, BoundedBuffer& buffer){
     for(int i = 0; i < 5; ++i){
         int value = buffer.fetch();
         std::cout << "Consumer " << id << " fetched " << value << std::endl;
         std::this_thread::sleep_for(std::chrono::milliseconds(250));
     }
 }
 
 void producer(int id, BoundedBuffer& buffer){
     for(int i = 0; i < 8; ++i){
         buffer.deposit(i);
         std::cout << "Produced " << id << " produced " << i << std::endl;
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
     }
 }

void f()
{
  int a=3;
  {
    int b=4;
    std::cout<<a<<" "<<b<<std::endl;
  }
  std::cout<<a<<" "<<b<<std::endl;
}


 
int main(){
     //f();
     /*
     BoundedBuffer buffer(15);
 
     std::thread c1(consumer, 0, std::ref(buffer));
     std::thread c2(consumer, 1, std::ref(buffer));
     std::thread c3(consumer, 2, std::ref(buffer));
     std::thread p1(producer, 0, std::ref(buffer));
     std::thread p2(producer, 1, std::ref(buffer));
 
     c1.join();
     c2.join();
     c3.join();
     p1.join();
     p2.join();
     */
     return 0;
}
