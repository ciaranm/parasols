/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_labelled_clique/tlcco_max_labelled_clique.hh>
#include <max_labelled_clique/print_incumbent.hh>
#include <max_labelled_clique/lcco_base.hh>

#include <cco/cco_mixin.hh>
#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>
#include <threads/queue.hh>
#include <threads/atomic_incumbent.hh>

#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>

using namespace parasols;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

namespace
{
    const constexpr int number_of_depths = 5;
    const constexpr int number_of_steal_points = number_of_depths - 1;

    struct Subproblem
    {
        std::vector<int> offsets;
    };

    struct QueueItem
    {
        Subproblem subproblem;
    };

    struct StealPoint
    {
        std::mutex mutex;
        std::condition_variable cv;

        bool is_finished;

        bool has_data;
        std::vector<int> data;

        bool was_stolen;

        StealPoint() :
            is_finished(false),
            has_data(false),
            was_stolen(false)
        {
            mutex.lock();
        }

        void publish(std::vector<int> & s)
        {
            if (is_finished)
                return;

            data = s;
            has_data = true;
            cv.notify_all();
            mutex.unlock();
        }

        bool steal(std::vector<int> & s) __attribute__((noinline))
        {
            std::unique_lock<std::mutex> guard(mutex);

            while ((! has_data) && (! is_finished))
                cv.wait(guard);

            if (! is_finished && has_data) {
                s = data;
                was_stolen = true;
                return true;
            }
            else
                return false;
        }

        bool unpublish_and_keep_going()
        {
            if (is_finished)
                return true;

            mutex.lock();
            has_data = false;
            return ! was_stolen;
        }

        void finished()
        {
            is_finished = true;
            has_data = false;
            cv.notify_all();
            mutex.unlock();
        }
    };

    struct alignas(16) StealPoints
    {
        std::vector<StealPoint> points;

        StealPoints() :
            points{ number_of_steal_points }
        {
        }
    };

    template <CCOPermutations perm_, unsigned size_, typename VertexType_>
    struct TLCCO :
        LCCOBase<perm_, size_, VertexType_, TLCCO<perm_, size_, VertexType_> >
    {
        using LCCOBase<perm_, size_, VertexType_, TLCCO<perm_, size_, VertexType_> >::LCCOBase;
        using LCCOBase<perm_, size_, VertexType_, TLCCO<perm_, size_, VertexType_> >::params;
        using LCCOBase<perm_, size_, VertexType_, TLCCO<perm_, size_, VertexType_> >::graph;
        using LCCOBase<perm_, size_, VertexType_, TLCCO<perm_, size_, VertexType_> >::order;
        using LCCOBase<perm_, size_, VertexType_, TLCCO<perm_, size_, VertexType_> >::expand;
        using LCCOBase<perm_, size_, VertexType_, TLCCO<perm_, size_, VertexType_> >::colour_class_order;

        AtomicIncumbent best_anywhere_bits; // global incumbent

        static unsigned make_best_anywhere_bits(unsigned c_popcount, unsigned l_popcount)
        {
            return (c_popcount << 16) | (~l_popcount & 0xffff);
        }

        auto run() -> MaxLabelledCliqueResult
        {
            best_anywhere_bits.update(make_best_anywhere_bits(params.initial_bound, 0));

            MaxLabelledCliqueResult global_result;
            global_result.size = params.initial_bound;
            std::mutex global_result_mutex;

            for (unsigned pass = 1 ; pass <= 2 ; ++pass) {
                print_position(params, "pass", std::vector<int>{ int(pass) });

                /* work queues */
                std::vector<std::unique_ptr<Queue<QueueItem> > > queues;
                for (unsigned depth = 0 ; depth < number_of_depths ; ++depth)
                    queues.push_back(std::unique_ptr<Queue<QueueItem> >{ new Queue<QueueItem>{ params.n_threads, false } });

                /* initial job */
                queues[0]->enqueue(QueueItem{ Subproblem{ std::vector<int>{} } });
                if (queues[0]->want_producer())
                    queues[0]->initial_producer_done();

                /* threads and steal points */
                std::list<std::thread> threads;
                std::vector<StealPoints> thread_steal_points(params.n_threads);

                // initial colouring
                std::array<VertexType_, size_ * bits_per_word> initial_p_order;
                std::array<VertexType_, size_ * bits_per_word> initial_colours;
                {
                    FixedBitSet<size_> initial_p;
                    initial_p.resize(graph.size());
                    initial_p.set_all();
                    colour_class_order(SelectColourClassOrderOverload<perm_>(), initial_p, initial_p_order, initial_colours);
                }

                /* workers */
                for (unsigned i = 0 ; i < params.n_threads ; ++i) {
                    threads.push_back(std::thread([&, i] {
                                auto start_time = steady_clock::now(); // local start time
                                auto overall_time = duration_cast<milliseconds>(steady_clock::now() - start_time);

                                MaxLabelledCliqueResult local_result; // local result

                                for (unsigned depth = 0 ; depth < number_of_depths ; ++depth) {
                                    if (queues[depth]->want_producer()) {
                                        /* steal */
                                        for (unsigned j = 0 ; j < params.n_threads ; ++j) {
                                            if (j == i)
                                                continue;

                                            std::vector<int> stole;
                                            if (thread_steal_points.at(j).points.at(depth - 1).steal(stole)) {
                                                print_position(params, "stole after", stole);
                                                stole.pop_back();
                                                for (auto & s : stole)
                                                    --s;
                                                while (++stole.back() < graph.size())
                                                    queues[depth]->enqueue(QueueItem{ Subproblem{ stole } });
                                            }
                                            else
                                                print_position(params, "did not steal", stole);
                                        }
                                        queues[depth]->initial_producer_done();
                                    }

                                    while (true) {
                                        // get some work to do
                                        QueueItem args;
                                        if (! queues[depth]->dequeue_blocking(args))
                                            break;

                                        print_position(params, "dequeued", args.subproblem.offsets);

                                        std::vector<VertexType_> c;
                                        c.reserve(graph.size());

                                        FixedBitSet<size_> p; // local potential additions
                                        p.resize(graph.size());
                                        p.set_all();

                                        std::vector<int> position;
                                        position.reserve(graph.size());
                                        position.push_back(0);

                                        LabelSet u;

                                        // do some work
                                        expand(pass == 2, c, p, u, initial_p_order, initial_colours, position, local_result,
                                                &args.subproblem, &thread_steal_points.at(i));

                                        // record the last time we finished doing useful stuff
                                        overall_time = duration_cast<milliseconds>(steady_clock::now() - start_time);
                                    }

                                    if (depth < number_of_steal_points)
                                        thread_steal_points.at(i).points.at(depth).finished();
                                }

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
            }

            return global_result;
        }

        auto potential_new_best(
                unsigned c_popcount,
                const std::vector<VertexType_> & c,
                unsigned cost,
                std::vector<int> & position,
                MaxLabelledCliqueResult & local_result,
                Subproblem * const,
                StealPoints * const)
            -> void
        {
            // potential new best
            if (best_anywhere_bits.update(make_best_anywhere_bits(c_popcount, cost))) {
                local_result.size = c_popcount;
                local_result.cost = cost;

                local_result.members.clear();
                for (auto & v : c)
                    local_result.members.insert(order[v]);

                print_incumbent(params, c_popcount, cost, position);
            }
        }

        auto recurse(
                bool pass_2,
                std::vector<VertexType_> & c,
                FixedBitSet<size_> & p,
                LabelSet & u,
                const std::array<VertexType_, size_ * bits_per_word> & p_order,
                const std::array<VertexType_, size_ * bits_per_word> & colours,
                std::vector<int> & position,
                MaxLabelledCliqueResult & local_result,
                Subproblem * const subproblem,
                StealPoints * const steal_points
                ) -> bool
        {
            unsigned c_popcount = c.size();

            if (steal_points && c_popcount < number_of_steal_points)
                steal_points->points.at(c_popcount - 1).publish(position);

            expand(pass_2, c, p, u, p_order, colours, position, local_result,
                subproblem && c_popcount < subproblem->offsets.size() ? subproblem : nullptr,
                steal_points && c_popcount < number_of_steal_points ? steal_points : nullptr);

            if (steal_points && c_popcount < number_of_steal_points)
                return steal_points->points.at(c_popcount - 1).unpublish_and_keep_going();
            else
                return true;
        }

        auto increment_nodes(
                MaxLabelledCliqueResult & local_result,
                Subproblem * const,
                StealPoints * const)
            -> void
        {
            ++local_result.nodes;
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return (best_anywhere_bits.get() >> 16) & 0xffff;
        }

        auto get_best_anywhere_cost() -> unsigned
        {
            return ~best_anywhere_bits.get() & 0xffff;
        }

        auto get_skip(unsigned c_popcount,
                MaxLabelledCliqueResult &,
                Subproblem * const subproblem,
                StealPoints * const,
                int & skip, bool & keep_going) -> void
        {
            if (subproblem && c_popcount < subproblem->offsets.size()) {
                skip = subproblem->offsets.at(c_popcount);
                keep_going = false;
            }
        }
    };
}

template <CCOPermutations perm_>
auto parasols::tlcco_max_labelled_clique(const Graph & graph, const MaxLabelledCliqueParams & params) -> MaxLabelledCliqueResult
{
    return select_graph_size<ApplyPerm<TLCCO, perm_>::template Type, MaxLabelledCliqueResult>(AllGraphSizes(), graph, params);
}

template auto parasols::tlcco_max_labelled_clique<CCOPermutations::None>(const Graph &, const MaxLabelledCliqueParams &) -> MaxLabelledCliqueResult;
template auto parasols::tlcco_max_labelled_clique<CCOPermutations::Defer1>(const Graph &, const MaxLabelledCliqueParams &) -> MaxLabelledCliqueResult;

