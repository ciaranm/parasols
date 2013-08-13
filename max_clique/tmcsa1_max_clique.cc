/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/tmcsa1_max_clique.hh>
#include <max_clique/colourise.hh>
#include <max_clique/queue.hh>
#include <max_clique/print_incumbent.hh>
#include <threads/atomic_incumbent.hh>
#include <graph/degree_sort.hh>

#include <algorithm>
#include <list>
#include <functional>
#include <vector>
#include <thread>

using namespace parasols;

namespace
{
    struct QueueItem
    {
        std::vector<int> c;
        std::vector<int> o;
        unsigned cn;
    };

    /**
     * We've possibly found a new best. Update best_anywhere and our local
     * result, and do any necessary printing.
     */
    auto found_possible_new_best(const std::vector<int> & c, const MaxCliqueParams & params,
            MaxCliqueResult & result, AtomicIncumbent & best_anywhere) -> void
    {
        if (best_anywhere.update(c.size())) {
            result.size = c.size();
            result.members = std::set<int>{ c.begin(), c.end() };
            print_incumbent(params, result.size);
        }
    }

    /**
     * Bound function.
     */
    auto bound(const std::vector<int> & c, int cn, const MaxCliqueParams & params, AtomicIncumbent & best_anywhere) -> bool
    {
        unsigned best_anywhere_value = best_anywhere.get();
        return (c.size() + cn <= best_anywhere_value || best_anywhere_value >= params.stop_after_finding);
    }

    auto expand(
            const Graph & graph,
            Buckets & buckets,                       // pre-allocated
            Queue<QueueItem> * const maybe_queue,    // not null if we're populating: enqueue here
            Queue<QueueItem> * const donation_queue, // not null if we're donating: donate here
            std::vector<int> & c,                    // current candidate clique
            std::vector<int> & o,                    // potential additions, in order
            MaxCliqueResult & result,
            const MaxCliqueParams & params,
            AtomicIncumbent & best_anywhere) -> void
    {
        ++result.nodes;

        // get our coloured vertices
        std::vector<int> p;
        p.reserve(o.size());
        auto colours = colourise(graph, buckets, p, o);

        bool chose_to_donate = false;

        // for each v in p... (v comes later)
        for (int n = p.size() - 1 ; n >= 0 ; --n) {

            // bound, timeout or early exit?
            if (bound(c, colours[n], params, best_anywhere) || params.abort.load())
                return;

            auto v = p[n];

            // consider taking v
            c.push_back(v);

            // filter o to contain vertices adjacent to v
            std::vector<int> new_o;
            new_o.reserve(o.size());
            std::copy_if(o.begin(), o.end(), std::back_inserter(new_o), [&] (int w) { return graph.adjacent(v, w); });

            if (new_o.empty()) {
                found_possible_new_best(c, params, result, best_anywhere);
            }
            else
            {
                // do we enqueue or recurse?
                bool should_expand = true;

                if (maybe_queue && c.size() == params.split_depth) {
                    maybe_queue->enqueue_blocking(QueueItem{ c, std::move(new_o), colours[n] }, params.n_threads);
                    should_expand = false;
                }
                else if (donation_queue && (chose_to_donate || donation_queue->want_donations())) {
                    donation_queue->enqueue(QueueItem{ c, std::move(new_o), colours[n] });
                    should_expand = false;
                    chose_to_donate = true;
                    ++result.donations;
                }

                if (should_expand)
                    expand(graph, buckets, maybe_queue, donation_queue, c, new_o, result, params, best_anywhere);
            }

            // now consider not taking v
            c.pop_back();
            p.pop_back();
            o.erase(std::remove(o.begin(), o.end(), v));
        }
    }
}

auto parasols::tmcsa1_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    std::vector<int> o(graph.size()); // potential additions, ordered
    std::iota(o.begin(), o.end(), 0);
    degree_sort(graph, o);

    Queue<QueueItem> queue{ params.n_threads, params.work_donation }; // work queue

    MaxCliqueResult result; // global result
    std::mutex result_mutex;

    AtomicIncumbent best_anywhere; // global incumbent
    best_anywhere.update(params.initial_bound);

    std::list<std::thread> threads; // populating thread, and workers

    /* populate */
    threads.push_back(std::thread([&] {
                MaxCliqueResult tr; // local result

                auto buckets = make_buckets(graph.size());

                std::vector<int> tc; // local candidate clique
                tc.reserve(graph.size());

                std::vector<int> to{ o }; // local potential additions, ordered

                // populate!
                expand(graph, buckets, &queue, nullptr, tc, to, tr, params, best_anywhere);

                // merge results
                queue.initial_producer_done();
                std::unique_lock<std::mutex> guard(result_mutex);
                result.merge(tr);
                }));

    /* workers */
    for (unsigned i = 0 ; i < params.n_threads ; ++i) {
        threads.push_back(std::thread([&, i] {
                    auto start_time = std::chrono::steady_clock::now(); // local start time

                    auto buckets = make_buckets(graph.size());

                    MaxCliqueResult tr; // local result

                    while (true) {
                        // get some work to do
                        QueueItem args;
                        if (! queue.dequeue_blocking(args))
                            break;

                        // re-evaluate the bound against our new best
                        if (args.cn <= best_anywhere.get())
                            continue;

                        // do some work
                        args.c.reserve(graph.size());
                        expand(graph, buckets, nullptr, params.work_donation ? &queue : nullptr,
                                args.c, args.o, tr, params, best_anywhere);

                        // keep track of top nodes done
                        if (! params.abort.load())
                            ++tr.top_nodes_done;
                    }

                    auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);

                    // merge results
                    {
                        std::unique_lock<std::mutex> guard(result_mutex);
                        result.merge(tr);
                        result.times.push_back(overall_time);
                    }
                    }));
    }

    // wait until they're done, and clean up threads
    for (auto & t : threads)
        t.join();

    return result;
}

