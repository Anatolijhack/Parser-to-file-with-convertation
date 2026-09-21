#pragma once
#include <iostream>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <vector>
#include <queue>
#include <future>
struct Task
{
	int priority = 0;
	int order = 0;
	std::function<void()> f;
	bool operator < (const Task& other) const
	{
		if (priority == other.priority)
		{
			return order > other.order;
		}
		return priority < other.priority;
	}
};
class ThreadPool;
template <typename In, typename Out>
class Stage : public std::enable_shared_from_this<Stage<In, Out>>
{
private:
	ThreadPool& pool;
	std::function<Out(In)> func;
	std::function<void(Out)> next;

public:
	Stage(ThreadPool& p, std::function<Out(In)> f)
		: pool(p), func(f) {}

	void set_next(std::function<void(Out)> n)
	{
		next = n;
	}

	void push(In value)
	{
		auto self = this->shared_from_this();

		pool.submit(0, [self, value]()
			{
				Out result = self->func(value);

				if (self->next)
					self->next(result);
			});
	}
};



	//Вход данных в стадию
	//void push(In value)
	//{
	//	auto current = this->shared_from_this();
	//	//this  доступ к next и func value  - копируеться  и иза єтого живет дальше
	//	//Лямбда передаеь задачу в ThreadPool
	//	//func = [](int x){ return x * 2; }
	//	//value = 5
	//    //result = 10
	//	pool.submit(0,[current, value]() {
	//		
	//		Out result = current->func(value);
	//		//если следующая стадия
	//		if (current->next)
	//		{
	//			//передаем результат дальше
	//			current->next(result);
	//		}
	//		});
	//}
class ThreadPool
{
private:
	std::mutex mtx;
	std::priority_queue<Task> questions;
	std::condition_variable ctx;
	std::condition_variable ctx2;
	std::condition_variable not_full;
	std::vector<std::thread> workers;
	int max_tsks = 50;

	int threadss = 10;
	int ques = 0;
	bool is = false; //остановка
	bool add = true; //модно ли добавить
	int works = 0;
	void DoWork()
	{
		while (true)
		{
			Task t;
			std::function<void()> func;

			{
				std::unique_lock<std::mutex> lock(mtx);

				ctx.wait(lock, [this]() 
					{
					return !questions.empty() || is;
					});

				if (is && questions.empty())
					return;

				t = questions.top();
				questions.pop();

				not_full.notify_one();

				
				works++;
			}


		    t.f();
			
	

			{
				std::lock_guard<std::mutex> lock(mtx);
				works--;

				not_full.notify_one();
			}
			ctx2.notify_all();

			
		}
	}
public:
	
	ThreadPool(size_t size)
	{
		for (size_t i = 0; i < size; i++)
		{
			workers.emplace_back([this]() {DoWork(); });
		}
	}
	void add_task(std::function<void()> question,int priority)
	{
		{
			std::lock_guard<std::mutex>lock(mtx);
			if (!add)
			{
				throw std::runtime_error("Thread pool stopped");
			}
			questions.push(Task{priority,ques++,question});
		}
		ctx.notify_one();
	}
	void shutdown()
	{
		{
			std::lock_guard<std::mutex> lock(mtx);

			if (is)
				return;

			add = false;
			is = true;
		}

		ctx.notify_all();
		ctx2.notify_all();

		for (auto& worker : workers)
		{
			if (worker.joinable())
				worker.join();
		}
	}
	
	void wait()
	{

		std::unique_lock<std::mutex> lock(mtx);
		{
			ctx2.wait(lock, [this]() {
				return (questions.empty() && works == 0);
				});
		}
		
	}
	//Класс F делает функцию универсальной для принятия lambda / function / functor Класс Args спопсобен принимать любое количество аргуметов
	//template <class F, class... Args>
	////Возвращает тип который выведет компилятор через ->  определяя lvakue , rvalue
	//auto submit(int priority,F&& f, Args&&... args)
	//	//вычисляет тип функции  
	//	-> std::future<std::invoke_result_t<F, Args...>>
	//{    
	//	
	//	//возвращаемый тип просто сокращаем в R
	//	using R = std::invoke_result_t<F, Args...>;
	//	//👉 это обёртка над функцией, которая:

	//	//вызывает функцию
	//		//сохраняет результат
	//		//отдаёт его future
	//	//std::bind превращает функцию в функцию без в аргументов
	//	//std::forward<F>(f) 👉 сохраняет тип (lvalue/rvalue)
	//	auto task = std::make_shared<std::packaged_task<R()>>(
	//		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	//	);
	//	//👉 получает future, связанный с packaged_task
	//	std::future<R> fut = task->get_future();

	//	std::unique_lock<std::mutex> lock(mtx);


	//	not_full.wait(lock, [this]() {return questions.size() < max_tsks || is; });

	//    if (!add || is)
	//	   throw std::runtime_error("Thread pool stopped");
	//		//[task] → capture shared_ptr
	//		//(*task)() → выполняем packaged_task
	//		//void();
	//	questions.push(Task{ priority,ques++,[task]() { (*task)(); }});

	//	lock.unlock();
	//	ctx.notify_one();
	//
	//	//вернуть future пользователю
	//	return fut;
	//}
	//template <class F, class... Args>
	//auto submit(int priority, F&& f, Args&&... args)
	//	-> std::future<std::invoke_result_t<F, Args...>>
	//{
	//	using R = std::invoke_result_t<F, Args...>;

	//	auto task = std::make_shared<std::packaged_task<R()>>(
	//		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	//	);

	//	std::future<R> fut = task->get_future();

	//	{
	//		std::lock_guard<std::mutex> lock(mtx);

	//		not_full.wait(lock, [this]()
	//			{
	//				return questions.size() < max_tsks || is;
	//			});

	//		if (!add)
	//			throw std::runtime_error("Thread pool stopped");

	//		questions.push(Task{
	//			priority,
	//			ques++,
	//			[task]() { (*task)(); }   // 🔥 ВОТ ЭТО КЛЮЧЕВОЕ
	//			});
	//	}

	//	ctx.notify_one();

	//	return fut;
	//}
	//template <class F, class... Args>
	//auto submit(int priority, F&& f, Args&&... args)
	//	-> std::future<std::invoke_result_t<F, Args...>>
	//{
	//	using R = std::invoke_result_t<F, Args...>;

	//	// сохраняем функцию и аргументы
	//	auto task = std::make_shared<std::packaged_task<R()>>(
	//		[func = std::forward<F>(f),
	//		argsTuple = std::make_tuple(std::forward<Args>(args)...)
	//		]() mutable -> R
	//		{
	//			return std::apply(std::move(func), std::move(argsTuple));
	//		}
	//	);

	//	std::future<R> fut = task->get_future();

	//	{
	//		std::unique_lock<std::mutex> lock(mtx);

	//		not_full.wait(lock, [this]()
	//			{
	//				return questions.size() < max_tsks || is;
	//			});

	//		if (!add || is)
	//			throw std::runtime_error("Thread pool stopped");

	//		questions.push(Task{
	//			priority,
	//			ques++,
	//			[task]() { (*task)(); }
	//			});
	//	}

	//	ctx.notify_one();

	//	return fut;
	//}
template <class F, class... Args>
auto submit(int priority, F&& f, Args&&... args)
-> std::future<std::invoke_result_t<F, Args...>>
{
	using R = std::invoke_result_t<F, Args...>;

	auto task = std::make_shared<std::packaged_task<R()>>(
		[func = std::forward<F>(f),
		argsTuple = std::make_tuple(std::forward<Args>(args)...)
		]() mutable -> R
		{
			return std::apply(std::move(func), std::move(argsTuple));
		}
	);

	std::future<R> fut = task->get_future();

	{
		std::unique_lock<std::mutex> lock(mtx);

		not_full.wait(lock, [this]()
			{
				return questions.size() < max_tsks || is;
			});

		if (!add || is)
			throw std::runtime_error("Thread pool stopped");

		questions.emplace(Task{
			priority,
			ques++,
			[task]() mutable   // 👈 ВАЖНО: НИКАКОГО MOVE
			{
				(*task)();
			}
			});
	}

	ctx.notify_one();
	return fut;
}
	~ThreadPool()
	{
		shutdown();
	}

};

