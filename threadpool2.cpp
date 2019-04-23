/*
  设想一个高并发的服务器处理程序，服务器要用多线程处理大数据量计算，然后将计算结果封装成消息放入队列中，
  然后另起几个线程专门负责处理消息队列中的消息分发给不同客户端，这样瓶颈就出来了，N多线程都在频繁锁消息队列，
  这样导致队里的利用效率下降一半，无论是入队还是出队都要对队列进行全部加锁，所以弄一个双锁[队头锁，队尾锁]队列。
*/

typedef struct node_t {
    TYPE value; 
    node_t *next
} NODE;
 
typedef struct queue_t {
    NODE *head; 
    NODE *tail;
    LOCK q_h_lock;
    LOCK q_t_lock;
} Q;
 
initialize(Q *q) {
   node = new_node()   // Allocate a free node
   node->next = NULL   // Make it the only node in the linked list
   q->head = q->tail = node   // Both head and tail point to it
   q->q_h_lock = q->q_t_lock = FREE   // Locks are initially free
}
 
enqueue(Q *q, TYPE value) {
   node = new_node()       // Allocate a new node from the free list
   node->value = value     // Copy enqueued value into node
   node->next = NULL       // Set next pointer of node to NULL
   lock(&q->q_t_lock)      // Acquire t_lock in order to access Tail
      q->tail->next = node // Link node at the end of the queue
      q->tail = node       // Swing Tail to node
   unlock(&q->q_t_lock)    // Release t_lock
｝
 
dequeue(Q *q, TYPE *pvalue) {
   lock(&q->q_h_lock)   // Acquire h_lock in order to access Head
      node = q->head    // Read Head
      new_head = node->next       // Read next pointer
      if new_head == NULL         // Is queue empty?
         unlock(&q->q_h_lock)     // Release h_lock before return
         return FALSE             // Queue was empty
      endif
      *pvalue = new_head->value   // Queue not empty, read value
      q->head = new_head  // Swing Head to next node
   unlock(&q->q_h_lock)   // Release h_lock
   free(node)             // Free node
   return TRUE            // Queue was not empty, dequeue succeeded
}
