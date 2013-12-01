/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/dbmcsa_max_clique.hh>
#include <max_clique/colourise.hh>
#include <max_clique/print_incumbent.hh>
#include <threads/atomic_incumbent.hh>
#include <threads/queue.hh>
#include <graph/degree_sort.hh>
#include <graph/min_width_sort.hh>

#include <algorithm>
#include <list>
#include <functional>
#include <vector>
#include <thread>

using namespace parasols;

namespace
{
    template <unsigned size_>
    struct QueueItem
    {
        FixedBitSet<size_> c;
        FixedBitSet<size_> p;
        unsigned cn;
        std::vector<int> position;
    };

    template <unsigned size_>
    struct StealPoint
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;
        FixedBitSet<size_> c;
        FixedBitSet<size_> p;
        std::vector<int> position;
        int skip = 0;
        bool was_stolen = false;

        void not_ready()
        {
            std::unique_lock<std::mutex> guard(mutex);
            ready = false;
            was_stolen = false;
            skip = 0;
        }

        void finished()
        {
            std::unique_lock<std::mutex> guard(mutex);
            ready = true;
            was_stolen = false;
            skip = 0;
            p = FixedBitSet<size_>();
            cv.notify_all();
        }
    };

    template <unsigned size_>
    auto found_possible_new_best(const FixedBitGraph<size_> & graph, const std::vector<int> & o,
            const FixedBitSet<size_> & c, int c_popcount,
            const MaxCliqueParams & params, MaxCliqueResult & result, AtomicIncumbent & best_anywhere,
            const std::vector<int> & position) -> void
    {
        if (best_anywhere.update(c_popcount)) {
            result.size = c_popcount;
            result.members.clear();
            for (int i = 0 ; i < graph.size() ; ++i)
                if (c.test(i))
                    result.members.insert(o[i]);
            print_incumbent(params, result.size, position);
        }
    }

    auto bound(unsigned c_popcount, unsigned cn, const MaxCliqueParams & params, AtomicIncumbent & best_anywhere) -> bool
    {
        unsigned best_anywhere_value = best_anywhere.get();
        return (c_popcount + cn <= best_anywhere_value || best_anywhere_value >= params.stop_after_finding);
    }

    template <MaxCliqueOrder order_, unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,                      // vertex ordering
            Queue<QueueItem<size_> > * const maybe_queue,    // not null if we're populating: enqueue here
            bool blocking_enqueue,
            StealPoint<size_> * const steal_point,
            StealPoint<size_> * const next_steal_point,
            FixedBitSet<size_> & c,                          // current candidate clique
            FixedBitSet<size_> & p,                          // potential additions
            int skip,
            MaxCliqueResult & result,
            const MaxCliqueParams & params,
            AtomicIncumbent & best_anywhere,
            std::vector<int> & position) -> void
    {
        ++result.nodes;

        auto c_popcount = c.popcount();

        // get our coloured vertices
        std::array<unsigned, size_ * bits_per_word> p_order, colours;
        colourise<size_>(graph, p, p_order, colours);

        // for each v in p... (v comes later)
        for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
            ++position.back();

            // bound, timeout or early exit?
            if (bound(c_popcount, colours[n], params, best_anywhere) || params.abort.load())
                return;

            auto v = p_order[n];

            // consider taking v
            c.set(v);
            ++c_popcount;

            if (skip > 0) {
                --skip;
            }
            else {
                // export stealable?
                if (steal_point) {
                    std::unique_lock<std::mutex> guard(steal_point->mutex);
                    if (steal_point->was_stolen)
                        return;

                    if (! steal_point->ready) {
                        steal_point->c = c;
                        steal_point->p = p;
                        steal_point->position = position;
                        steal_point->skip = 0;
                        steal_point->was_stolen = false;
                        steal_point->ready = true;
                        steal_point->cv.notify_all();
                    }

                    ++steal_point->skip;
                }

                // filter p to contain vertices adjacent to v
                FixedBitSet<size_> new_p = p;
                new_p = p;
                graph.intersect_with_row(v, new_p);

                if (new_p.empty())
                    found_possible_new_best(graph, o, c, c_popcount, params, result, best_anywhere, position);
                else
                {
                    if (maybe_queue) {
                        auto new_position = position;
                        new_position.push_back(0);
                        if (blocking_enqueue)
                            maybe_queue->enqueue_blocking(QueueItem<size_>{ c, std::move(new_p), c_popcount + colours[n], std::move(new_position) }, params.n_threads);
                        else
                            maybe_queue->enqueue(QueueItem<size_>{ c, std::move(new_p), c_popcount + colours[n], std::move(new_position) });
                    }
                    else {
                        position.push_back(0);
                        if (next_steal_point)
                            next_steal_point->not_ready();
                        expand<order_, size_>(graph, o, maybe_queue, false,
                                next_steal_point, nullptr,
                                c, new_p, 0, result, params, best_anywhere, position);
                        if (next_steal_point)
                            next_steal_point->not_ready();
                        position.pop_back();
                    }
                }
            }

            // now consider not taking v
            c.unset(v);
            p.unset(v);
            --c_popcount;
        }
    }

    template <MaxCliqueOrder order_, unsigned size_>
    auto max_clique(const FixedBitGraph<size_> & graph, const std::vector<int> & o, const MaxCliqueParams & params) -> MaxCliqueResult
    {
        Queue<QueueItem<size_> > queue{ params.n_threads, false, false }; // work queue
        Queue<QueueItem<size_> > queue_2{ params.n_threads, false, false }; // work queue, depth 2
        Queue<QueueItem<size_> > queue_3{ params.n_threads, false, false }; // work queue, depth 3

        MaxCliqueResult result; // global result
        std::mutex result_mutex;

        AtomicIncumbent best_anywhere; // global incumbent
        best_anywhere.update(params.initial_bound);

        std::list<std::thread> threads; // populating thread, and workers

        /* populate */
        threads.push_back(std::thread([&] {
                    MaxCliqueResult tr; // local result

                    FixedBitSet<size_> tc; // local candidate clique
                    tc.resize(graph.size());

                    FixedBitSet<size_> tp; // local potential additions
                    tp.resize(graph.size());
                    tp.set_all();

                    std::vector<int> position;
                    position.reserve(graph.size());
                    position.push_back(0);

                    // populate!
                    expand<order_, size_>(graph, o, &queue, true, nullptr, nullptr,
                        tc, tp, 0, result, params, best_anywhere, position);

                    // merge results
                    queue.initial_producer_done();
                    std::unique_lock<std::mutex> guard(result_mutex);
                    result.merge(tr);
                    }));

        /* steal points */
        std::vector<StealPoint<size_> > steal_points_1((params.n_threads));
        std::vector<StealPoint<size_> > steal_points_2((params.n_threads));

        /* workers */
        for (unsigned i = 0 ; i < params.n_threads ; ++i) {
            threads.push_back(std::thread([&, i] {
                        auto start_time = std::chrono::steady_clock::now(); // local start time

                        MaxCliqueResult tr; // local result

                        auto * current_queue = &queue, next_queue = &queue_2, next_next_queue = &queue_3;
                        auto * current_steal_points = &steal_points_1, next_steal_points = &steal_points_2;

                        while (true) {
                            while (true) {
                                // get some work to do
                                QueueItem<size_> args;
                                if (! current_queue->dequeue_blocking(args))
                                    break;

                                // re-evaluate the bound against our new best
                                if (args.cn <= best_anywhere.get())
                                    continue;

                                if (current_steal_points)
                                    (*current_steal_points)[i].not_ready();
                                if (next_steal_points)
                                    (*next_steal_points)[i].not_ready();

                                // do some work
                                expand<order_, size_>(graph, o, nullptr, false,
                                        current_steal_points ? &(*current_steal_points)[i] : nullptr,
                                        next_steal_points ? &(*next_steal_points)[i] : nullptr,
                                        args.c, args.p, 0, tr, params, best_anywhere, args.position);

                                if (current_steal_points)
                                    (*current_steal_points)[i].not_ready();
                                if (next_steal_points)
                                    (*next_steal_points)[i].not_ready();
                            }

                            if (current_steal_points)
                                (*current_steal_points)[i].finished();
                            if (next_steal_points)
                                (*next_steal_points)[i].finished();

                            if (! next_queue)
                                break;

                            if (next_queue->want_producer()) {
                                for (auto & s : *current_steal_points) {
                                    std::unique_lock<std::mutex> guard(s.mutex);
                                    while (! s.ready)
                                        s.cv.wait(guard);

                                    s.was_stolen = true;
                                    auto c = s.c;
                                    auto p = s.p;
                                    auto skip = s.skip;
                                    auto position = s.position;
                                    guard.unlock();

                                    expand<order_, size_>(graph, o, next_queue, false,
                                            nullptr, nullptr,
                                            c, p, skip, tr, params, best_anywhere, position);
                                }

                                next_queue->initial_producer_done();
                            }

                            current_queue = next_queue;
                            next_queue = next_next_queue;
                            next_next_queue = nullptr;
                            current_steal_points = next_steal_points;
                            next_steal_points = nullptr;
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

    template <MaxCliqueOrder order_, unsigned size_>
    auto dbmcsa(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
    {
        std::vector<int> o(graph.size()); // vertex ordering
        std::iota(o.begin(), o.end(), 0);

        switch (order_) {
            case MaxCliqueOrder::Degree:
                degree_sort(graph, o, false);
                break;
            case MaxCliqueOrder::MinWidth:
                min_width_sort(graph, o, false);
                break;
            case MaxCliqueOrder::ExDegree:
                exdegree_sort(graph, o, false);
                break;
            case MaxCliqueOrder::DynExDegree:
                dynexdegree_sort(graph, o, false);
                break;
        }

        // re-encode graph as a bit graph
        FixedBitGraph<size_> bit_graph;
        bit_graph.resize(graph.size());

        for (int i = 0 ; i < graph.size() ; ++i)
            for (int j = 0 ; j < graph.size() ; ++j)
                if (graph.adjacent(o[i], o[j]))
                    bit_graph.add_edge(i, j);

        // go!
        return max_clique<order_>(bit_graph, o, params);
    }
}

template <MaxCliqueOrder order_>
auto parasols::dbmcsa_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 1024, "Need to update here if max_graph_size is changed.");
    if (graph.size() < bits_per_word)
        return dbmcsa<order_, 1>(graph, params);
    else if (graph.size() < 2 * bits_per_word)
        return dbmcsa<order_, 2>(graph, params);
    else if (graph.size() < 4 * bits_per_word)
        return dbmcsa<order_, 4>(graph, params);
    else if (graph.size() < 8 * bits_per_word)
        return dbmcsa<order_, 8>(graph, params);
    else if (graph.size() < 16 * bits_per_word)
        return dbmcsa<order_, 16>(graph, params);
    else if (graph.size() < 32 * bits_per_word)
        return dbmcsa<order_, 32>(graph, params);
    else if (graph.size() < 64 * bits_per_word)
        return dbmcsa<order_, 64>(graph, params);
    else if (graph.size() < 128 * bits_per_word)
        return dbmcsa<order_, 128>(graph, params);
    else if (graph.size() < 256 * bits_per_word)
        return dbmcsa<order_, 256>(graph, params);
    else if (graph.size() < 512 * bits_per_word)
        return dbmcsa<order_, 512>(graph, params);
    else if (graph.size() < 1024 * bits_per_word)
        return dbmcsa<order_, 1024>(graph, params);
    else
        throw GraphTooBig();
}

template auto parasols::dbmcsa_max_clique<MaxCliqueOrder::Degree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::dbmcsa_max_clique<MaxCliqueOrder::MinWidth>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::dbmcsa_max_clique<MaxCliqueOrder::ExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::dbmcsa_max_clique<MaxCliqueOrder::DynExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

