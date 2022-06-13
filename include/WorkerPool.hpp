#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <tuple>
#include <utility>
#include <functional>
#include <iostream>

namespace trv
{
	template <typename... Types>
	class WorkerPool
	{
	public:
		typedef void(*Task)(Types...);
		WorkerPool(Task task, size_t threadCount = std::thread::hardware_concurrency()) :
			_task(task)
		{
			for (size_t i = 0; i < threadCount; ++i)
			{
				_workers.emplace_back(std::thread(&WorkerPool::_task_wrapper, this));
			}
		};

		~WorkerPool()
		{
			std::unique_lock lock(_mtx);
			_shouldTerminate = true;
			lock.unlock();
			_cv.notify_all();
			for (auto& worker : _workers)
			{
				worker.join();
			}
		}

		WorkerPool() = delete;
		WorkerPool(WorkerPool& other) = delete;
		WorkerPool(WorkerPool&&) = delete;
		WorkerPool& operator=(WorkerPool&) = delete;
		WorkerPool& operator=(WorkerPool&&) = delete;

		void AddTask(Types... args)
		{
			std::unique_lock lock(_mtx);
			_tasks.emplace(std::forward<Types>(args)...);
			lock.unlock();
			_cv.notify_one();
		}

		void Stop()
		{
			std::unique_lock lock(_mtx);
			_shouldTerminate = true;
			_tasks = {};
			lock.unlock();
			_cv.notify_all();
			for (auto& worker : _workers)
			{
				worker.join();
			}
		}

		bool Busy()
		{
			std::lock_guard lock(_mtx);
			return !_tasks.empty();
		}

	private:
		void _task_wrapper()
		{
			while (true)
			{
				std::unique_lock lock(_mtx);
				while (_tasks.empty() && !_shouldTerminate)
				{
					_cv.wait(lock);
				}
				if (_shouldTerminate)
				{
					return;
				}

				std::size_t id = std::hash<std::thread::id>{}(std::this_thread::get_id());
				std::cout << id << " woke up.\n";
				std::tuple<Types...> args = std::move(_tasks.front());
				_tasks.pop();
				lock.unlock();
				std::apply(_task, args);
				std::cout << id << " finished.\n";
			}
		};

		bool _shouldTerminate = false;
		std::mutex _mtx;
		std::condition_variable _cv;
		Task _task;
		std::vector<std::thread> _workers;
		std::queue<std::tuple<Types...>> _tasks;
	};
}