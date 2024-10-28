#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class Thread
{
public:
	Thread() {
		worker = std::thread(&Thread::QueueLoop, this);
	}

	~Thread() {
		if (worker.joinable())
		{
			Wait();
			queueMutex.lock();
			destroying = true;
			condition.notify_one();
			queueMutex.unlock();
			worker.join();
		}
	}

	// Add a new job to the thread's queue
	void AddJob(std::function<void()> function)
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		jobQueue.push(std::move(function));
		condition.notify_one();
	}

	// Wait until all work items have been finished
	void Wait()
	{
		std::unique_lock<std::mutex> lock(queueMutex);
		condition.wait(lock, [this]() { return jobQueue.empty(); });
	}

private:
	bool destroying = false;
	std::thread worker;
	std::queue<std::function<void()>> jobQueue;
	std::mutex queueMutex;
	std::condition_variable condition;

	// Loop through all remaining jobs
	void QueueLoop()
	{
		while (true)
		{
			std::function<void()> job;
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				condition.wait(lock, [this] { return !jobQueue.empty() || destroying; });
				if (destroying)
				{
					break;
				}
				job = jobQueue.front();
			}

			job();

			{
				std::lock_guard<std::mutex> lock(queueMutex);
				jobQueue.pop();
				condition.notify_one();
			}
		}
	}
};

class ThreadPool
{
public:
	std::vector<std::unique_ptr<Thread>> threads;

	// Sets the number of threads to be allocated in this pool
	void SetThreadCount(uint32_t count)
	{
		threads.clear();
		for (uint32_t i = 0; i < count; i++)
			threads.push_back(std::make_unique<Thread>());
	}

	// Wait until all threads have finished their work items
	void Wait()
	{
		for (auto& thread : threads)
			thread->Wait();
	}
};
