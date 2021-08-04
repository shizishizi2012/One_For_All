#ifndef THREADSAFE_QUEUE_H
#define THREADSAFE_QUEUE_H

#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

/**
 * @brief Thread safe thread queue
 * 
 * @tparam T 
 * @version 0.1
 * @author shizishizi2012 (921270269@qq.com)
 * @date 2021-08-04
 * @copyright JayChan (c) 2021
 */
template<typename T>
class ThreadsafeQueue
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    std::condition_variable data_cond;

    node* get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head() {
        std::unique_ptr<node> old_head=std::move(head);
        head=std::move(old_head->next);
        return old_head;
    }

    std::unique_lock<std::mutex> wait_for_data() {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock,[&]{return head.get()!=get_tail();});
        return std::move(head_lock); // 3
    }

    std::unique_ptr<node> wait_pop_head() {
        std::unique_lock<std::mutex> head_lock(wait_for_data()); // 4
        return pop_head();
    }

    std::unique_ptr<node> wait_pop_head(T& value) {
        std::unique_lock<std::mutex> head_lock(wait_for_data()); // 5
        value=std::move(*head->data);
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if(head.get()==get_tail()) {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(T& value) {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if(head.get()==get_tail()) {
            return std::unique_ptr<node>();
        }
        value=std::move(*head->data);
        return pop_head();
    }
public:
    /**
     * @brief Construct a new Threadsafe Queue object
     * 
     * @version 0.1
     * @author shizishizi2012 (921270269@qq.com)
     * @date 2021-08-04
     * @copyright JayChan (c) 2021
     */
    ThreadsafeQueue(): head(new node),tail(head.get())
    {}

    ThreadsafeQueue(const ThreadsafeQueue& other)=delete;
    ThreadsafeQueue& operator=(const ThreadsafeQueue& other)=delete;

    /**
     * @brief 非阻塞的列表弹出
     * 
     * @return std::shared_ptr<T> 
     * @version 0.1
     * @author shizishizi2012 (921270269@qq.com)
     * @date 2021-08-04
     * @copyright JayChan (c) 2021
     */
    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> old_head=try_pop_head();
        return old_head?old_head->data:std::shared_ptr<T>();
    }

    /**
     * @brief 非阻塞的列表弹出
     * 
     * @param value 
     * @return true 
     * @return false 
     * @version 0.1
     * @author shizishizi2012 (921270269@qq.com)
     * @date 2021-08-04
     * @copyright JayChan (c) 2021
     */
    bool try_pop(T& value) {
        std::unique_ptr<node> const old_head=try_pop_head(value);
        return old_head;
    }

    /**
     * @brief 阻塞的列表弹出
     * 
     * @return std::shared_ptr<T> 
     * @version 0.1
     * @author shizishizi2012 (921270269@qq.com)
     * @date 2021-08-04
     * @copyright JayChan (c) 2021
     */
    std::shared_ptr<T> wait_and_pop() {
        std::unique_ptr<node> const old_head=wait_pop_head();
        return old_head->data;
    }

    /**
     * @brief 阻塞的列表弹出
     * 
     * @param value 
     * @version 0.1
     * @author shizishizi2012 (921270269@qq.com)
     * @date 2021-08-04
     * @copyright JayChan (c) 2021
     */
    void wait_and_pop(T& value) {
        std::unique_ptr<node> const old_head=wait_pop_head(value);
    }

    /**
     * @brief 阻塞的推入列表
     * 
     * @param new_value 
     * @version 0.1
     * @author shizishizi2012 (921270269@qq.com)
     * @date 2021-08-04
     * @copyright JayChan (c) 2021
     */
    void push(T new_value) {
        std::shared_ptr<T> new_data(
            std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data=new_data;
            node* const new_tail=p.get();
            tail->next=std::move(p);
            tail=new_tail;
        }
        data_cond.notify_one();
    }

    /**
     * @brief 判断队列是否为空
     * 
     * @return true 
     * @return false 
     * @version 0.1
     * @author shizishizi2012 (921270269@qq.com)
     * @date 2021-08-04
     * @copyright JayChan (c) 2021
     */
    bool empty() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return (head.get()==get_tail());
    }
};

#endif
