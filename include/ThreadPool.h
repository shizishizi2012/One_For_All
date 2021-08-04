#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

#include "threadsafe_queue.hpp"

class JoinThreads
{
	std::vector<std::thread>& threads;
public:
	explicit JoinThreads(std::vector<std::thread>& threads_): threads(threads_) 
	{}
	~JoinThreads() { // 保存
		for(unsigned long i=0;i<threads.size();++i) {
			if(threads[i].joinable())
				threads[i].join();
		}
	}
};


class FunctionWrapper {
	struct impl_base {
		virtual void call()=0;
		virtual ~impl_base() {}
	};
	
	std::unique_ptr<impl_base> impl;
	template<typename F>
	struct impl_type: impl_base {
		F f;
		impl_type(F&& f_): f(std::move(f_)) {}
		void call() { f(); }
	};
public:
	template<typename F>
	FunctionWrapper(F&& f): impl(new impl_type<F>(std::move(f)))
	{}
	
	void operator()() { impl->call(); }
	
	FunctionWrapper() = default;
	
	FunctionWrapper(FunctionWrapper&& other): impl(std::move(other.impl))
	{}
	
	FunctionWrapper& operator=(FunctionWrapper&& other) {
		impl=std::move(other.impl);
		return *this;
	}
	FunctionWrapper(const FunctionWrapper&)=delete;
	FunctionWrapper(FunctionWrapper&)=delete;
	FunctionWrapper& operator=(const FunctionWrapper&)=delete;
};


class ThreadPool {
	std::atomic_bool done;
	ThreadsafeQueue<FunctionWrapper> work_queue;
	std::vector<std::thread> threads;
	JoinThreads joiner;
	void worker_thread() {
		while(!done) {
			FunctionWrapper task; // 使用function_wrapper，而非std::function
			if(work_queue.try_pop(task)) {
				task();
			} else {
				std::this_thread::yield();
			}
		}
	}
public:
	ThreadPool():done(false), joiner(threads) {
		unsigned const thread_count=std::thread::hardware_concurrency(); // 
		try {
			for(unsigned i=0;i<thread_count;++i) {
				threads.push_back(std::thread(&ThreadPool::worker_thread,this)); // 
			}
		} catch(...) {
			done=true; // 
			throw;
		}
	}
	~ThreadPool() {
		done=true;
	}

	template<typename FunctionType>
	std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f) {
		typedef typename std::result_of<FunctionType()>::type result_type; // 2
		std::packaged_task<result_type()> task(std::move(f)); // 3
		std::future<result_type> res(task.get_future()); // 4
		work_queue.push(std::move(task)); // 5
		return res; // 6
	}
};

#endif
