#pragma once

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <tuple>
#include <utility>
#include <vector>

namespace trv
{
template <typename... Types>
class WorkerPool
{
   public:
	typedef void (*Task)(Types...);
	WorkerPool(Task task, std::size_t threadCount = std::thread::hardware_concurrency()) :
	    _task(task)
	{
		for (size_t i = 0; i < threadCount; ++i)
		{
			_workers.emplace_back(std::bind(&WorkerPool::_task_wrapper, this));
		}
	};

	~WorkerPool()
	{
		std::unique_lock lock(_mtx);
		_shouldTerminate = true;
		lock.unlock();
		_cv_task.notify_all();
		for (auto& worker : _workers)
		{
			worker.join();
		}
	}

	WorkerPool()                        = delete;
	WorkerPool(WorkerPool& other)       = delete;
	WorkerPool(WorkerPool&&)            = delete;
	WorkerPool& operator=(WorkerPool&)  = delete;
	WorkerPool& operator=(WorkerPool&&) = delete;

	void AddTask(Types... args)
	{
		std::unique_lock lock(_mtx);
		_tasks.emplace(std::forward<Types>(args)...);
		_cv_task.notify_one();
	}

	void Stop()
	{
		std::unique_lock lock(_mtx);
		_shouldTerminate = true;
		_tasks           = {};
		lock.unlock();
		_cv_task.notify_all();
		for (auto& worker : _workers)
		{
			worker.join();
		}
	}

	void WaitUntilFinished()
	{
		std::unique_lock lock(_mtx);
		_cv_finished.wait(lock, [this]() { return _tasks.empty() && _active == 0; });
	}

   private:
	void _task_wrapper()
	{
		while (true)
		{
			std::unique_lock lock(_mtx);
			_cv_task.wait(lock, [this]() { return _shouldTerminate || !_tasks.empty(); });

			if (_shouldTerminate)
			{
				return;
			}

			++_active;
			std::tuple<Types...> args = std::move(_tasks.front());
			_tasks.pop();
			lock.unlock();

			std::apply(_task, args);

			lock.lock();
			--_active;
			_cv_finished.notify_one();
		}
	};

	bool _shouldTerminate = false;
	std::mutex _mtx;
	std::condition_variable _cv_task;
	std::condition_variable _cv_finished;
	Task _task;
	std::vector<std::thread> _workers;
	std::queue<std::tuple<Types...>> _tasks;
	int _active = 0;
};
}
