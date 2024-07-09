#pragma once
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

using std::thread;
using std::unique_lock;
using std::condition_variable;
using std::mutex;
using std::atomic;
using std::vector;
using std::queue;
using std::lock_guard;
using std::unique_lock;
using std::function;
using std::future;
using std::make_shared;
using std::forward;
using std::packaged_task;


//Threadpool class
class Threadpool {
private:
    int m_threads;
    vector<thread>threads;
    queue<function<void()>> tasks; //for execution of tasks in FIFO manner
    mutex mtx;
    condition_variable cv;
    bool stop;

public:
    explicit Threadpool(int numThreads) :m_threads(numThreads), stop(false) {
        for (int i = 0; i < m_threads; i++) {
            //storing threads
            threads.emplace_back([this] { //lambda function for threads. This is initial function for the threads
                function<void()> task;
                while (1) {
                    unique_lock<mutex> lock(mtx);
                    cv.wait(lock, [this] {
                        return !tasks.empty() || stop; //if there are no tasks OR the pool is going to end. Put the thread to sleep
                        });

                    if (stop) {
                        return;
                    }

                    task = move(tasks.front());
                    tasks.pop();

                    lock.unlock();
                    task();
                }
                });
        }
    }

    ~Threadpool() {
        unique_lock<mutex> lock(mtx);
        stop = true; // dont take any further tasks;

        lock.unlock();
        cv.notify_all();

        for (auto& th : threads) {
            th.join();
        }
    }

    template<class F, class... Args>
    auto ExecuteTask(F&& f, Args&&... args) -> future<decltype (f(args...))> {

        using return_type = decltype(f(args...));

        auto task = make_shared<packaged_task<return_type()>>(bind(forward<F>(f), forward<Args>(args)...)); //shared pointer

        future<return_type> res = task->get_future();

        unique_lock<mutex>lock(mtx);

        tasks.emplace([task]() -> void {
            (*task)();
            });

        lock.unlock();
        cv.notify_one();

        return res;
    }
};