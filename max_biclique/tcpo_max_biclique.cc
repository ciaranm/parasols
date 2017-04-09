/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_biclique/tcpo_max_biclique.hh>
#include <max_biclique/cpo_base.hh>
#include <max_biclique/print_incumbent.hh>

#include <threads/atomic_incumbent.hh>
#include <threads/queue.hh>

#include <graph/template_voodoo.hh>

#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>

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

    template <CCOPermutations perm_, BicliqueSymmetryRemoval sym_, unsigned size_, typename VertexType_>
    struct TCPO : CPOBase<perm_, sym_, size_, VertexType_, TCPO<perm_, sym_, size_, VertexType_> >
    {
        using Base = CPOBase<perm_, sym_, size_, VertexType_, TCPO<perm_, sym_, size_, VertexType_> >;

        using Base::graph;
        using Base::original_graph;
        using Base::params;
        using Base::expand;
        using Base::order;
        using Base::colour_class_order;

        AtomicIncumbent best_anywhere; // global incumbent

        std::list<std::set<int> > previouses;
        std::mutex previouses_mutex;

        TCPO(const Graph & g, const MaxBicliqueParams & p) :
            Base(g, p)
        {
        }

        auto run() -> MaxBicliqueResult
        {
            best_anywhere.update(params.initial_bound);

            MaxBicliqueResult global_result;
            global_result.size = params.initial_bound;
            std::mutex global_result_mutex;

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
            FixedBitSet<size_> initial_p;
            initial_p.set_up_to(graph.size());
            colour_class_order(SelectColourClassOrderOverload<perm_>(), initial_p, initial_p_order, initial_colours);

            /* workers */
            for (unsigned i = 0 ; i < params.n_threads ; ++i) {
                threads.push_back(std::thread([&, i] {
                            auto start_time = steady_clock::now(); // local start time
                            auto overall_time = duration_cast<milliseconds>(steady_clock::now() - start_time);

                            MaxBicliqueResult local_result; // local result

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

                                    std::vector<unsigned> ca, cb;
                                    ca.reserve(graph.size());
                                    cb.reserve(graph.size());

                                    FixedBitSet<size_> pa, pb; // local potential additions
                                    pa.set_up_to(graph.size());
                                    pb.set_up_to(graph.size());

                                    std::vector<int> position;
                                    position.reserve(graph.size());
                                    position.push_back(0);

                                    FixedBitSet<size_> sym_skip;

                                    // do some work
                                    expand(ca, cb, pa, pb, sym_skip, initial_p_order, initial_colours, position, local_result,
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
                                global_result.merge(std::move(local_result));
                                global_result.times.push_back(overall_time);
                            }
                            }));
            }

            // wait until they're done, and clean up threads
            for (auto & t : threads)
                t.join();

            return global_result;
        }

        auto increment_nodes(
                MaxBicliqueResult & local_result,
                Subproblem * const,
                StealPoints * const
                ) -> void
        {
            ++local_result.nodes;
        }

        auto recurse(
                std::vector<unsigned> & ca,
                std::vector<unsigned> & cb,
                FixedBitSet<size_> & pa,
                FixedBitSet<size_> & pb,
                FixedBitSet<size_> & sym_skip,
                const std::array<VertexType_, size_ * bits_per_word> & pa_order,
                const std::array<VertexType_, size_ * bits_per_word> & pa_bounds,
                std::vector<int> & position,
                MaxBicliqueResult & local_result,
                Subproblem * const subproblem,
                StealPoints * const steal_points
                ) -> bool
        {
            if (steal_points && (ca.size() + cb.size()) < number_of_steal_points)
                steal_points->points.at(ca.size() + cb.size() - 1).publish(position);

            expand(ca, cb, pa, pb, sym_skip, pa_order, pa_bounds, position, local_result,
                subproblem && (ca.size() + cb.size()) < subproblem->offsets.size() ? subproblem : nullptr,
                steal_points && (ca.size() + cb.size()) < number_of_steal_points ? steal_points : nullptr);

            if (steal_points && (ca.size() + cb.size()) < number_of_steal_points)
                return steal_points->points.at(ca.size() + cb.size() - 1).unpublish_and_keep_going();
            else
                return true;
        }

        auto potential_new_best(
                const std::vector<unsigned> & ca,
                const std::vector<unsigned> & cb,
                const std::vector<int> & position,
                MaxBicliqueResult & local_result,
                Subproblem * const,
                StealPoints * const
                ) -> void
        {
            if (ca.size() == cb.size()) {
                if (best_anywhere.update(ca.size())) {
                    local_result.size = ca.size();
                    local_result.members_a.clear();
                    for (auto & v : ca)
                        local_result.members_a.insert(order[v]);
                    local_result.members_b.clear();
                    for (auto & v : cb)
                        local_result.members_b.insert(order[v]);
                    print_incumbent(params, local_result.size, position);
                }
            }
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return best_anywhere.get();
        }

        auto get_skip_and_stop(
                unsigned c_popcount,
                MaxBicliqueResult &,
                Subproblem * const subproblem,
                StealPoints * const,
                int & skip,
                int &,
                bool & keep_going
                ) -> void
        {
            if (subproblem && c_popcount < subproblem->offsets.size()) {
                skip = subproblem->offsets.at(c_popcount);
                keep_going = false;
            }
        }
    };
}

template <CCOPermutations perm_, BicliqueSymmetryRemoval sym_>
auto parasols::tcpo_max_biclique(const Graph & graph, const MaxBicliqueParams & params) -> MaxBicliqueResult
{
    return select_graph_size<ApplyPermSym<TCPO, perm_, sym_>::template Type, MaxBicliqueResult>(AllGraphSizes(), graph, params);
}

template auto parasols::tcpo_max_biclique<CCOPermutations::None, BicliqueSymmetryRemoval::None>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;
template auto parasols::tcpo_max_biclique<CCOPermutations::Defer1, BicliqueSymmetryRemoval::None>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;

template auto parasols::tcpo_max_biclique<CCOPermutations::None, BicliqueSymmetryRemoval::Remove>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;
template auto parasols::tcpo_max_biclique<CCOPermutations::Defer1, BicliqueSymmetryRemoval::Remove>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;

template auto parasols::tcpo_max_biclique<CCOPermutations::None, BicliqueSymmetryRemoval::Skip>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;

