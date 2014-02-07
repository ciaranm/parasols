/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_biclique/dccd_max_biclique.hh>
#include <max_biclique/print_incumbent.hh>
#include <max_biclique/clique_cover.hh>

#include <graph/bit_graph.hh>
#include <graph/degree_sort.hh>

#include <threads/atomic_incumbent.hh>
#include <threads/queue.hh>

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
        FixedBitSet<size_> ca;
        FixedBitSet<size_> cb;
        FixedBitSet<size_> pa;
        FixedBitSet<size_> pb;
        std::vector<int> position;
    };

    template <unsigned size_>
    auto found_possible_new_best(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,
            const FixedBitSet<size_> & ca,
            const FixedBitSet<size_> & cb,
            int ca_popcount,
            const MaxBicliqueParams & params,
            MaxBicliqueResult & result,
            AtomicIncumbent & best_anywhere,
            const std::vector<int> & position) -> void
    {
        if (best_anywhere.update(ca_popcount)) {
            result.size = ca_popcount;
            result.members_a.clear();
            result.members_b.clear();
            for (int i = 0 ; i < graph.size() ; ++i) {
                if (ca.test(i))
                    result.members_a.insert(o[i]);
                if (cb.test(i))
                    result.members_b.insert(o[i]);
            }

            print_incumbent(params, result.size, position);
        }
    }

    template <unsigned size_>
    auto naive_expand(
            const FixedBitGraph<size_> & graph,
            const MaxBicliqueParams & params,
            Queue<QueueItem<size_> > * const maybe_queue,    // not null if we're populating: enqueue here
            bool blocking_enqueue,
            MaxBicliqueResult & result,
            AtomicIncumbent & best_anywhere,
            const std::vector<int> & o,
            FixedBitSet<size_> & ca,
            FixedBitSet<size_> & cb,
            FixedBitSet<size_> & pa,
            FixedBitSet<size_> & pb,
            std::vector<int> & position
            ) -> void
    {
        ++result.nodes;

        unsigned ca_popcount = ca.popcount();
        unsigned cb_popcount = cb.popcount();
        unsigned pa_popcount = pa.popcount();
        unsigned pb_popcount = pb.popcount();

        // for each v in pa...
        while (! pa.empty()) {
            ++position.back();

            // timeout or early exit?
            if (result.size >= params.stop_after_finding || params.abort.load())
                return;

            // bound
            unsigned best_anywhere_value = best_anywhere.get();
            if (pa_popcount + ca_popcount <= best_anywhere_value)
                return;
            if (pb_popcount + cb_popcount <= best_anywhere_value)
                return;

            // consider taking v
            int v = pa.last_set_bit();

            ca.set(v);
            ++ca_popcount;
            pa.unset(v);
            --pa_popcount;

            // filter p to contain vertices adjacent to v
            FixedBitSet<size_> new_pa = pa, new_pb = pb;
            graph.intersect_with_row_complement(v, new_pa);
            graph.intersect_with_row(v, new_pb);

            // potential new best
            if (ca_popcount == cb_popcount && ca_popcount > result.size)
                found_possible_new_best(graph, o, ca, cb, ca_popcount, params, result, best_anywhere, position);

            if (! new_pb.empty()) {
                /* swap a and b */

                if (maybe_queue) {
                    auto new_position = position;
                    new_position.push_back(0);
                    if (blocking_enqueue)
                        maybe_queue->enqueue_blocking(
                                QueueItem<size_>{ cb, ca, std::move(new_pb), std::move(new_pa), std::move(new_position) },
                                params.n_threads);
                    else
                        maybe_queue->enqueue(
                                QueueItem<size_>{ cb, ca, std::move(new_pb), std::move(new_pa), std::move(new_position) });
                }
                else {
                    position.push_back(0);
                    naive_expand(graph, params, maybe_queue, blocking_enqueue, result, best_anywhere, o, cb, ca, new_pb, new_pa, position);
                    position.pop_back();
                }
            }

            // now consider not taking v
            ca.unset(v);
            --ca_popcount;

            // if cb is empty, do not take cb = { v }
            if (params.break_ab_symmetry) {
                if (cb.empty()) {
                    pb.unset(v);
                    pb_popcount = pb.popcount();
                }
            }
        }
    }

    template <unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const MaxBicliqueParams & params,
            Queue<QueueItem<size_> > * const maybe_queue,    // not null if we're populating: enqueue here
            bool blocking_enqueue,
            MaxBicliqueResult & result,
            AtomicIncumbent & best_anywhere,
            const std::vector<int> & o,
            FixedBitSet<size_> & ca,
            FixedBitSet<size_> & cb,
            FixedBitSet<size_> & pa,
            FixedBitSet<size_> & pb,
            bool pa_is_independent,
            bool pb_is_independent,
            std::vector<int> & position
            ) -> void
    {
        ++result.nodes;

        std::array<unsigned, size_ * bits_per_word> pa_order, cliques;
        clique_cover<size_>(graph, pa, pa_order, cliques);

        unsigned ca_popcount = ca.popcount();
        unsigned cb_popcount = cb.popcount();
        unsigned pa_popcount = pa.popcount();
        unsigned pb_popcount = pb.popcount();

        // for each v in pa...
        for (int n = pa_popcount - 1 ; n >= 0 ; --n) {
            ++position.back();

            // timeout or early exit?
            if (result.size >= params.stop_after_finding || params.abort.load())
                return;

            // bound
            unsigned best_anywhere_value = best_anywhere.get();
            if (cliques[n] + ca_popcount <= best_anywhere_value)
                return;
            if (pb_popcount + cb_popcount <= best_anywhere_value)
                return;

            bool new_pa_is_independent = pa_is_independent || ((n > 1) && (cliques[n] == unsigned(n + 1)));

            // consider taking v
            int v = pa_order[n];

            ca.set(v);
            ++ca_popcount;
            pa.unset(v);

            // filter pb to contain vertices adjacent to v, and pa to contain
            // vertices not adjacent to v
            FixedBitSet<size_> new_pa = pa, new_pb = pb;
            graph.intersect_with_row_complement(v, new_pa);
            graph.intersect_with_row(v, new_pb);

            // potential new best
            if (ca_popcount == cb_popcount && ca_popcount > result.size)
                found_possible_new_best(graph, o, ca, cb, ca_popcount, params, result, best_anywhere, position);

            if (! new_pb.empty()) {
                /* swap a and b */

                if (maybe_queue) {
                    auto new_position = position;
                    new_position.push_back(0);
                    if (blocking_enqueue)
                        maybe_queue->enqueue_blocking(
                                QueueItem<size_>{ cb, ca, std::move(new_pb), std::move(new_pa), std::move(new_position) },
                                params.n_threads);
                    else
                        maybe_queue->enqueue(
                                QueueItem<size_>{ cb, ca, std::move(new_pb), std::move(new_pa), std::move(new_position) });
                }
                else {
                    position.push_back(0);
                    //if (new_pa_is_independent && pb_is_independent)
                    //    naive_expand(graph, params, maybe_queue, blocking_enqueue, result, best_anywhere, o, cb, ca, new_pb, new_pa, position);
                    //else
                        expand(graph, params, maybe_queue, blocking_enqueue,
                                result, best_anywhere, o, cb, ca, new_pb, new_pa, pb_is_independent, new_pa_is_independent, position);
                    position.pop_back();
                }
            }

            // now consider not taking v
            ca.unset(v);
            --ca_popcount;

            // if cb is empty, do not take cb = { v }
            if (params.break_ab_symmetry) {
                if (cb.empty()) {
                    pb.unset(v);
                    pb_popcount = pb.popcount();
                }
            }
        }
    }

    template <unsigned size_>
    auto max_biclique(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,
            const MaxBicliqueParams & params) -> MaxBicliqueResult
    {
        Queue<QueueItem<size_> > queue{ params.n_threads, false, false }; // work queue

        MaxBicliqueResult result; // global result
        std::mutex result_mutex;

        AtomicIncumbent best_anywhere; // global incumbent
        best_anywhere.update(params.initial_bound);

        std::list<std::thread> threads; // populating thread, and workers

        /* populate */
        threads.push_back(std::thread([&] {
                    MaxBicliqueResult tr; // local result

                    FixedBitSet<size_> tca, tcb; // local candidate clique
                    tca.resize(graph.size());
                    tcb.resize(graph.size());

                    FixedBitSet<size_> tpa, tpb; // local potential additions
                    tpa.resize(graph.size());
                    tpa.set_all();
                    tpb.resize(graph.size());
                    tpb.set_all();

                    std::vector<int> position;
                    position.reserve(graph.size());
                    position.push_back(0);

                    // populate!
                    expand<size_>(graph, params, &queue, true, result, best_anywhere, o, tca, tcb, tpa, tpb, false, false, position);

                    // merge results
                    queue.initial_producer_done();
                    std::unique_lock<std::mutex> guard(result_mutex);
                    result.merge(std::move(tr));
                    }));

        /* workers */
        for (unsigned i = 0 ; i < params.n_threads ; ++i) {
            threads.push_back(std::thread([&, i] {
                        auto start_time = std::chrono::steady_clock::now(); // local start time

                        MaxBicliqueResult tr; // local result

                        while (true) {
                            // get some work to do
                            QueueItem<size_> args;
                            if (! queue.dequeue_blocking(args))
                                break;

                            // do some work
                            expand<size_>(graph, params, nullptr, false, tr, best_anywhere, o,
                                args.ca, args.cb, args.pa, args.pb, false, false, args.position);
                        }

                        auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);

                        // merge results
                        {
                            std::unique_lock<std::mutex> guard(result_mutex);
                            result.merge(std::move(tr));
                            result.times.push_back(overall_time);
                        }
                        }));
        }

        // wait until they're done, and clean up threads
        for (auto & t : threads)
            t.join();

        return result;
    }

    template <unsigned size_>
    auto dccd(const Graph & graph, const MaxBicliqueParams & params) -> MaxBicliqueResult
    {
        MaxBicliqueResult result;
        result.size = params.initial_bound;

        std::vector<int> o(graph.size()); // vertex ordering
        std::iota(o.begin(), o.end(), 0);
        degree_sort(graph, o, false);

        // re-encode graph as a bit graph
        FixedBitGraph<size_> bit_graph;
        bit_graph.resize(graph.size());

        for (int i = 0 ; i < graph.size() ; ++i)
            for (int j = 0 ; j < graph.size() ; ++j)
                if (graph.adjacent(o[i], o[j]))
                    bit_graph.add_edge(i, j);

        return max_biclique(bit_graph, o, params);
    }
}

auto parasols::dccd_max_biclique(const Graph & graph, const MaxBicliqueParams & params) -> MaxBicliqueResult
{
    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 1024, "Need to update here if max_graph_size is changed.");
    if (graph.size() < bits_per_word)
        return dccd<1>(graph, params);
    else if (graph.size() < 2 * bits_per_word)
        return dccd<2>(graph, params);
    else if (graph.size() < 4 * bits_per_word)
        return dccd<4>(graph, params);
    else if (graph.size() < 8 * bits_per_word)
        return dccd<8>(graph, params);
    else if (graph.size() < 16 * bits_per_word)
        return dccd<16>(graph, params);
    else if (graph.size() < 32 * bits_per_word)
        return dccd<32>(graph, params);
    else if (graph.size() < 64 * bits_per_word)
        return dccd<64>(graph, params);
    else if (graph.size() < 128 * bits_per_word)
        return dccd<128>(graph, params);
    else if (graph.size() < 256 * bits_per_word)
        return dccd<256>(graph, params);
    else if (graph.size() < 512 * bits_per_word)
        return dccd<512>(graph, params);
    else if (graph.size() < 1024 * bits_per_word)
        return dccd<1024>(graph, params);
    else
        throw GraphTooBig();
}

