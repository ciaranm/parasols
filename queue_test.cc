/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#define _GLIBCXX_USE_NANOSLEEP 1

#include <clique/queue.hh>
#include <iostream>
#include <thread>
#include <vector>
#include <list>

using namespace clique;

struct FoundABug
{
};

/* Sneaky hack for synced output */
struct LockOutput
{
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock;
    LockOutput() : lock(mutex) { }
};

std::mutex LockOutput::mutex;

std::ostream & operator<< (std::ostream & s, const LockOutput &) { return s; }

int main(int, char *[])
{
    const unsigned n_threads = std::thread::hardware_concurrency();

    /* Our 'work' is to replace tasks[n] with n */
    std::vector<unsigned> tasks(220);

    /* Each queue item is a half-open range of task numbers. */
    Queue<std::pair<int, int> > queue(n_threads, true);

    std::list<std::thread> threads;

    /* populator */
    threads.push_back(std::thread([&queue, &tasks, n_threads] {
                /* Initially, split tasks into 10-sized chunks */
                for (unsigned i = 0 ; i < tasks.size() ; i += 10) {
                    std::cerr << LockOutput() << std::this_thread::get_id() << " (populator) enqueuing " << i << std::endl;
                    queue.enqueue_blocking(std::make_pair(i, i + 10), n_threads);
                }

                queue.initial_producer_done();
                }));

    /* consumers */
    for (unsigned i = 0 ; i < n_threads ; ++i)
        threads.push_back(std::thread([&queue, &tasks, i] {
                    while (true) {
                        std::cerr << LockOutput() << std::this_thread::get_id() << " asking for something to do" << std::endl;

                        std::pair<int, int> p;
                        if (! queue.dequeue_blocking(p))
                            break;

                        std::cerr << LockOutput() << std::this_thread::get_id() << " told to do " << p.first << " to " << p.second << std::endl;

                        bool donate_remainder = false;
                        for (int n = p.first ; n != p.second ; ++n) {
                            if (0 != tasks[n])
                                throw FoundABug();

                            if (queue.want_donations()) {
                                /* never donate a 1-range */
                                if (p.first + 1 != p.second)
                                    donate_remainder = true;
                            }

                            if (donate_remainder) {
                                std::cerr << LockOutput() << std::this_thread::get_id() << " donating " << n << std::endl;
                                queue.enqueue(std::make_pair(n, n + 1));
                            }
                            else {
                                std::cerr << LockOutput() << std::this_thread::get_id() << " doing " << n << std::endl;
                                std::this_thread::sleep_for(std::chrono::milliseconds((i + 1) * 100));
                                tasks[n] = n;
                            }
                        }
                    }

                    std::cerr << LockOutput() << std::this_thread::get_id() << " finished" << std::endl;
                    }));

    for (auto & t : threads)
        t.join();

    for (unsigned i = 0 ; i < tasks.size() ; ++i)
        if (i != tasks[i])
            throw FoundABug();
}

