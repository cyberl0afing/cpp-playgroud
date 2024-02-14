#include "iostream"
#include "mutex"
#include "condition_variable"
#include <queue>
#include <vector>
#include <algorithm>
#include <thread>
#include <functional>
#include <vector>

std::once_flag once; 

class ThreadPool {
	//single instance
private:
	
public:
	explicit ThreadPool(int n_threads) :stop(false) {
		stop = false;
		for (size_t i = 0; i < n_threads; i++)
		{
			m_pool.emplace_back(std::thread([this]() {
				//
				while (true) {
					//从队列中取任务即可
					std::unique_lock<std::mutex> lck(mtx); // already lock //实际上是操作缓冲区的加锁
					//wait 
					m_cv.wait(lck, [this]() {
							return !m_task.empty() || stop; 
						});//pre // 暂时放弃这个锁，等待唤醒 

					if (m_task.empty() && stop)return;

					//get task
					std::function<void()> t(std::move(m_task.front()));//move const more faster
					m_task.pop();

					lck.unlock();

					t();
					//this time could unlock the queue actually...
				}
				}));
		}
	}
	static ThreadPool *  getInstance(int n_threads) {
		static ThreadPool* tp = nullptr; 
		std::call_once(once, [&]() {
			if (tp == nullptr) {
					tp = new ThreadPool(n_threads);
				}
			});
		return tp; 
	}
	
	
	~ThreadPool() {
		std::unique_lock<std::mutex> lck(mtx);
		stop = true;  
		m_cv.notify_all();//没有人再去使用了直接执行所有线程
		lck.unlock();
		for (auto& t : m_pool) {
			t.join();
		}

		std::cout << "threadPool decons" << std::endl;

	}
	template<class F, class ...Args>
	void enqueue(F && func , Args &&... args) {
		//同样的，对缓冲区进行加锁，放入之后通知一个人来进行操作就完了
		std::function<void()> task = 
			std::bind(std::forward<F>(func), std::forward<Args>(args)...);
		{
			std::unique_lock<std::mutex> lck(mtx);
			m_task.emplace(std::move(task));//wan mei zhuanfa
		}

		m_cv.notify_one();
	}
private:
		std::condition_variable m_cv;

		std::mutex mtx; 
		std::vector<std::thread> m_pool; 
		std::queue<std::function<void()>> m_task;
		bool stop; 
};

int main() {
	ThreadPool tp(4);
	for (int i = 0; i < 10; i++)
	{
		tp.enqueue([]() {
				std::cout << "Helloworld:" << std::endl;
			});
	}


	return 0; 
}
