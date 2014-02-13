/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/tcco_max_clique.hh>
#include <max_clique/cco_base.hh>
#include <max_clique/print_incumbent.hh>

#include <threads/queue.hh>
#include <threads/atomic_incumbent.hh>

#include <algorithm>
#include <thread>
#include <mutex>

using namespace parasols;

namespace
{
    struct Skips
    {
        std::vector<int> start_at;
    };

    template <unsigned size_>
    struct QueueItem
    {
        Skips skips;
    };

    template <CCOPermutations perm_, unsigned size_>
    struct TCCO : CCOBase<perm_, size_, TCCO<perm_, size_> >
    {
        using CCOBase<perm_, size_, TCCO<perm_, size_> >::CCOBase;

        using CCOBase<perm_, size_, TCCO<perm_, size_> >::graph;
        using CCOBase<perm_, size_, TCCO<perm_, size_> >::params;
        using CCOBase<perm_, size_, TCCO<perm_, size_> >::expand;
        using CCOBase<perm_, size_, TCCO<perm_, size_> >::order;

        AtomicIncumbent best_anywhere; // global incumbent

        auto run() -> MaxCliqueResult
        {
            best_anywhere.update(params.initial_bound);

            MaxCliqueResult global_result;
            global_result.size = params.initial_bound;
            std::mutex global_result_mutex;

            Queue<QueueItem<size_> > queue{ params.n_threads, false }; // work queue

            std::list<std::thread> threads; // populating thread, and workers

            /* populate */
            threads.push_back(std::thread([&] {
                        MaxCliqueResult local_result; // local result

                        FixedBitSet<size_> c; // local candidate clique
                        c.resize(graph.size());

                        FixedBitSet<size_> p; // local potential additions
                        p.resize(graph.size());
                        p.set_all();

                        std::vector<int> position;
                        position.reserve(graph.size());
                        position.push_back(0);

                        Skips skips;

                        // populate!
                        expand(c, p, position, &queue, local_result, &skips);

                        // merge results
                        queue.initial_producer_done();
                        std::unique_lock<std::mutex> guard(global_result_mutex);
                        global_result.merge(local_result);
            }));

            /* workers */
            for (unsigned i = 0 ; i < params.n_threads ; ++i) {
                threads.push_back(std::thread([&, i] {
                            auto start_time = std::chrono::steady_clock::now(); // local start time

                            MaxCliqueResult local_result; // local result

                            while (true) {
                                // get some work to do
                                QueueItem<size_> args;
                                if (! queue.dequeue_blocking(args))
                                    break;

                                print_position(params, "dequeued", args.skips.start_at);

                                FixedBitSet<size_> c; // local candidate clique
                                c.resize(graph.size());

                                FixedBitSet<size_> p; // local potential additions
                                p.resize(graph.size());
                                p.set_all();

                                std::vector<int> position;
                                position.reserve(graph.size());
                                position.push_back(0);

                                // do some work
                                expand(c, p, position, nullptr, local_result, &args.skips);
                            }

                            auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);

                            // merge results
                            {
                                std::unique_lock<std::mutex> guard(global_result_mutex);
                                global_result.merge(local_result);
                                global_result.times.push_back(overall_time);
                            }
                            }));
            }

            // wait until they're done, and clean up threads
            for (auto & t : threads)
                t.join();

            return global_result;
        }

        auto incremement_nodes(
                Queue<QueueItem<size_> > * const,
                MaxCliqueResult & local_result,
                Skips * const) -> void
        {
            ++local_result.nodes;
        }

        auto recurse(
                FixedBitSet<size_> & c,
                FixedBitSet<size_> & p,
                std::vector<int> & position,
                Queue<QueueItem<size_> > * const maybe_queue,
                MaxCliqueResult & local_result,
                Skips * const skips
                ) -> void
        {
            if (maybe_queue) {
                auto start_at = position;
                start_at.pop_back();
                maybe_queue->enqueue(QueueItem<size_>{ std::move(start_at) });
            }
            else
                expand(c, p, position, nullptr, local_result, skips);
        }

        auto potential_new_best(
                unsigned c_popcount,
                const FixedBitSet<size_> & c,
                std::vector<int> & position,
                Queue<QueueItem<size_> > * const,
                MaxCliqueResult & local_result,
                Skips * const
                ) -> void
        {
            if (best_anywhere.update(c_popcount)) {
                local_result.size = c_popcount;
                local_result.members.clear();
                for (int i = 0 ; i < graph.size() ; ++i)
                    if (c.test(i))
                        local_result.members.insert(order[i]);
                print_incumbent(params, local_result.size, position);
            }
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return best_anywhere.get();
        }

        auto initialise_skip(
                int & skip,
                bool & skip_was_nonzero,
                unsigned c_popcount,
                Queue<QueueItem<size_> > * const,
                MaxCliqueResult &,
                Skips * const skips) -> void
        {
            if (skips && skips->start_at.size() > c_popcount) {
                skip = skips->start_at.at(c_popcount);
                skip_was_nonzero = true;
                --skip;
            }
        }
    };
}

template <CCOPermutations perm_>
auto parasols::tcco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPerm<TCCO, perm_>::template Type, MaxCliqueResult>(AllGraphSizes(), graph, params);
}

template auto parasols::tcco_max_clique<CCOPermutations::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::tcco_max_clique<CCOPermutations::Defer1>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::tcco_max_clique<CCOPermutations::Sort>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

