#ifndef FUT_H
#define FUT_H

#include <memory>
#include <future>
#include <list>
#include <thread>
#include <condition_variable>

namespace fut {

	template<typename F, typename... Args>
	using async_result = std::result_of_t<std::decay_t<F>(std::decay_t<Args>...)>;

    struct packaged_job_base {
        virtual void operator()() = 0;
    };

    template<typename F, typename... Args>
    struct packaged_job : packaged_job_base {
        std::packaged_task<F> task;
        std::tuple<Args...> args;
        bool started;

        packaged_job(F&& f, Args&&... args) : task(std::forward<F>(f)), args(std::forward<Args>(args)...), started(false) {}

        std::future<async_result<F, Args...>> get_future() { return task.get_future(); }

        template<size_t... Is>
        void call_impl(std::index_sequence<Is...>) { task(std::get<Is>(args)...); }

        virtual void operator()() override { call_impl(std::make_index_sequence<sizeof...(Args)>()); }
    };

    template<size_t MaxJobs = 2>
    struct background_executor {
        std::list<std::unique_ptr<packaged_job_base>> job_queue;
        std::mutex job_queue_mut;
        std::condition_variable job_queue_cv;
        size_t active_job_count = 0;

        background_executor() {
            std::thread t([&] {
                std::unique_lock<std::mutex> lock(job_queue_mut);

                while (true) { // TODO stop
                    job_queue_cv.wait(lock, [&]{ return job_queue.size() > 0 && active_job_count < MaxJobs; });
                    start_job();
                }
            });
            t.detach();
        }

        void enqueue(std::unique_ptr<packaged_job_base> job) {
            std::unique_lock<std::mutex> lock(job_queue_mut);

            lock.lock();
            job_queue.push_back(job);
            lock.unlock();

            job_queue_cv.notify_one();
        }

        static void run_job(std::unique_ptr<packaged_job_base> job, std::condition_variable& cv, std::mutex& mut, size_t& active_job_count) {
            (*job)();

            std::unique_lock<std::mutex> lock(mut);
            lock.lock();
            active_job_count--;
            lock.unlock();

            cv.notify_one();
        }

        void start_job() {
            std::thread t(run_job, job_queue.pop_front(), job_queue_cv);
            t.detach();
            active_job_count++;
        }

        //void run_admin() {
        //    std::unique_lock<std::mutex> lock(job_queue_mut);

        //    while (true) { // TODO stop
        //        job_queue_cv.wait(lock, [&]{ return job_queue.size() > 0 && active_job_count < MaxJobs; });
        //        start_job();
        //    }
        //}
    };

    enum class launch { sync, deferred, background, async };

    template<typename F, typename... Args>
    std::future<async_result<F, Args...>> async(launch policy, F&& f, Args&&... args) {
        auto job = packaged_job(std::forward<F>(f), std::forward<Args>(args)...);
        auto res = job.get_future();

        switch (policy) {
            case launch::sync: job(); break;
            case launch::deferred: throw "not yet implemented"; break;
            case launch::background: throw "not yet implemented"; break;
            case launch::async: throw "not yet implemented"; break;
            default: throw "invalid fut::launch policy"; break;
        }

        return res;
    }

}

#endif
