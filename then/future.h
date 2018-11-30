#ifndef FUTURE_H
#define FUTURE_H

#include <condition_variable>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <optional>

namespace then {
    template<typename ValueType>
    struct shared_state {
        std::optional<ValueType> value;
        std::exception_ptr exception;
        mutable std::condition_variable cond_var;
        mutable std::mutex mutex;
        bool ready;

        shared_state() : ready(false) {}

        void set_value(ValueType const& value) {
            this->value = value;
            cond_var.notify_one();
        }

        void set_value(ValueType && value) {
            this->value = std::move(value);
            cond_var.notify_one();
        }

        void set_exception(std::exception_ptr exception) {
            this->exception = exception;
            cond_var.notify_one();
        }

        bool is_ready() const {
            return value || exception;
        }

        void wait() const {
            std::unique_lock<std::mutex> lock(mutex);
            cond_var.wait(lock, [this] { return is_ready(); });
        }

        template<class Rep, class Period>
        std::future_status wait_for(std::chrono::duration<Rep, Period> const & timeout_duration) const {
            std::unique_lock<std::mutex> lock(mutex);

            std::cv_status status = cond_var.wait_for(lock, timeout_duration, [this] { return is_ready(); });

            if (status == std::cv_status::timeout) return std::future_status::timeout;
            else return std::future_status::ready;
        }

        template<class Clock, class Duration>
        std::future_status wait_until(std::chrono::time_point<Clock, Duration> const & timeout_time) const {
            std::unique_lock<std::mutex> lock(mutex);

            std::cv_status status = cond_var.wait_until(lock, timeout_time, [this] { return is_ready(); });

            if (status == std::cv_status::timeout) return std::future_status::timeout;
            else return std::future_status::ready;
        }

        ValueType get() {
            wait();
            if (exception) std::rethrow_exception(exception);
            else return std::move(value.value());
        }
    };

    template<typename ResultType> struct promise;

    template<typename ValueType>
    struct future {
        public:
            future() {}
            future(future const & other) = delete;
            future(future && other) {
                state = std::move(other.state);
            }

            future& operator=(future const & other) = delete;
            future& operator=(future&& other) {
                state = std::move(other.state);
            }

            ValueType get() { return state->get(); }

            bool valid() const { return std::atomic_load(&state); }

            void wait() const { state->wait(); }

            template<class Rep, class Period>
            std::future_status wait_for(std::chrono::duration<Rep, Period> const & timeout_duration) const {
                return state->wait_for(timeout_duration);
            }

            template<class Clock, class Duration>
            std::future_status wait_until(std::chrono::time_point<Clock, Duration> const & timeout_time) const {
                return state->wait_until(timeout_time);
            }

        //private:
            std::shared_ptr<shared_state<ValueType>> state;

            void set(std::shared_ptr<shared_state<ValueType>> state) {
                std::atomic_store(&(this->state), std::move(state));
            }

            friend class promise<ValueType>;
    };

    template<typename ResultType>
    struct promise {
        public:
            promise() {}
            promise(promise const & other) = delete;
            promise(promise && other) {
                state = std::move(other.state);
                future = std::move(other.future);
            }

            promise& operator=(promise const & other) = delete;
            promise& operator=(promise && other) {
                state = std::move(other.state);
                future = std::move(other.future);
            }

            future<ResultType> get_future() {
                return std::move(future);
            }

            void set_value(ResultType const & value) {
                // TODO
            }

        private:
            std::shared_ptr<shared_state<ResultType>> state;
            std::unique_ptr<future<ResultType>> future;
    };

}

#endif
