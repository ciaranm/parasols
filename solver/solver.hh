/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SOLVER_SOLVER_HH
#define PARASOLS_GUARD_SOLVER_SOLVER_HH 1

#include <string>
#include <thread>
#include <condition_variable>
#include <chrono>

namespace parasols
{
    /* Helper: return a function that runs the specified algorithm, dealing
     * with timing information and timeouts. */
    template <typename Result_, typename Params_, typename Graph_>
    auto run_this(Result_ func(const Graph_ &, const Params_ &)) -> std::function<Result_ (const Graph_ &, Params_ &, bool &, int)>
    {
        return [func] (const Graph_ & graph, Params_ & params, bool & aborted, int timeout) -> Result_ {
            /* For a timeout, we use a thread and a timed CV. We also wake the
             * CV up if we're done, so the timeout thread can terminate. */
            std::thread timeout_thread;
            std::mutex timeout_mutex;
            std::condition_variable timeout_cv;
            params.abort.store(false);
            if (0 != timeout) {
                timeout_thread = std::thread([&] {
                        auto abort_time = std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
                        {
                            /* Sleep until either we've reached the time limit,
                             * or we've finished all the work. */
                            std::unique_lock<std::mutex> guard(timeout_mutex);
                            while (! params.abort.load()) {
                                if (std::cv_status::timeout == timeout_cv.wait_until(guard, abort_time)) {
                                    /* We've woken up, and it's due to a timeout. */
                                    aborted = true;
                                    break;
                                }
                            }
                        }
                        params.abort.store(true);
                        });
            }

            /* Start the clock */
            params.start_time = std::chrono::steady_clock::now();
            auto result = (*func)(graph, params);

            /* Clean up the timeout thread */
            if (timeout_thread.joinable()) {
                {
                    std::unique_lock<std::mutex> guard(timeout_mutex);
                    params.abort.store(true);
                    timeout_cv.notify_all();
                }
                timeout_thread.join();
            }

            return result;
        };
    };
}

#endif
