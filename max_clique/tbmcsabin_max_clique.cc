/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/tbmcsabin_max_clique.hh>
#include <max_clique/colourise.hh>
#include <max_clique/print_incumbent.hh>
#include <threads/atomic_incumbent.hh>
#include <threads/queue.hh>
#include <graph/degree_sort.hh>

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
        std::array<unsigned, size_ * bits_per_word> p_order;
        std::array<unsigned, size_ * bits_per_word> colours;
        std::vector<int> position;
    };

    /**
     * We've possibly found a new best. Update best_anywhere and our local
     * result, and do any necessary printing.
     */
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

    /**
     * Bound function.
     */
    auto bound(unsigned c_popcount, unsigned cn, const MaxCliqueParams & params, AtomicIncumbent & best_anywhere) -> bool
    {
        unsigned best_anywhere_value = best_anywhere.get();
        return (c_popcount + cn <= best_anywhere_value || best_anywhere_value >= params.stop_after_finding);
    }

    template <unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,                                  // static vertex ordering
            Queue<QueueItem<size_> > & queue,
            const std::array<unsigned, size_ * bits_per_word> & p_order,
            const std::array<unsigned, size_ * bits_per_word> & colours,
            FixedBitSet<size_> & c,                                      // current candidate clique
            FixedBitSet<size_> & p,                                      // potential additions
            MaxCliqueResult & result,
            const MaxCliqueParams & params,
            AtomicIncumbent & best_anywhere,
            std::vector<int> & position) -> void
    {
        auto c_popcount = c.popcount();

        // for each v in p... (v comes later)
        int n = p.popcount() - 1;

        // bound, timeout or early exit?
        if (bound(c_popcount, colours[n], params, best_anywhere) || params.abort.load())
            return;

        auto v = p_order[n];

        // queue empty? enqueue "not taking v"
        bool enqueued_not_v = false;
        if (n > 5 && queue.want_donations() && ! bound(c_popcount, colours[n - 1], params, best_anywhere)) {
            enqueued_not_v = true;
            auto alt_p = p;
            alt_p.unset(v);
            auto alt_position = position;
            ++alt_position.back();
            queue.enqueue(QueueItem<size_>{ c, alt_p, p_order, colours, alt_position });
        }

        // consider taking v
        c.set(v);
        ++c_popcount;
        ++position.back();

        // filter p to contain vertices adjacent to v
        FixedBitSet<size_> new_p = p;
        new_p = p;
        graph.intersect_with_row(v, new_p);

        if (new_p.empty()) {
            found_possible_new_best(graph, o, c, c_popcount, params, result, best_anywhere, position);
        }
        else
        {
            std::array<unsigned, size_ * bits_per_word> new_p_order, new_colours;
            colourise<size_>(graph, new_p, new_p_order, new_colours);
            ++result.nodes; // rough consistency...

            position.push_back(0);
            expand<size_>(graph, o, queue, new_p_order, new_colours, c, new_p, result, params, best_anywhere, position);
            position.pop_back();
        }

        // now consider not taking v
        c.unset(v);
        p.unset(v);
        --c_popcount;

        if (n > 0 && ! enqueued_not_v) {
            expand<size_>(graph, o, queue, p_order, colours, c, p, result, params, best_anywhere, position);
        }
    }

    template <unsigned size_>
    auto max_clique(const FixedBitGraph<size_> & graph, const std::vector<int> & o, const MaxCliqueParams & params) -> MaxCliqueResult
    {
        Queue<QueueItem<size_> > queue{ params.n_threads, true }; // work queue

        MaxCliqueResult result; // global result
        std::mutex result_mutex;

        AtomicIncumbent best_anywhere; // global incumbent
        best_anywhere.update(params.initial_bound);

        std::list<std::thread> threads; // populating thread, and workers

        /* initial job */
        {
            FixedBitSet<size_> tc; // local candidate clique
            tc.resize(graph.size());

            FixedBitSet<size_> tp; // local potential additions
            tp.resize(graph.size());
            tp.set_all();

            std::vector<int> position;
            position.push_back(0);

            std::array<unsigned, size_ * bits_per_word> p_order, colours;
            colourise<size_>(graph, tp, p_order, colours);
            queue.enqueue(QueueItem<size_>{ tc, tp, p_order, colours, position });
            queue.initial_producer_done();
        }

        /* workers */
        for (unsigned i = 0 ; i < params.n_threads ; ++i) {
            threads.push_back(std::thread([&, i] {
                        auto start_time = std::chrono::steady_clock::now(); // local start time

                        MaxCliqueResult tr; // local result

                        while (true) {
                            // get some work to do
                            QueueItem<size_> args;
                            if (! queue.dequeue_blocking(args))
                                break;

                            // do some work
                            expand<size_>(graph, o, queue, args.p_order, args.colours, args.c, args.p, tr, params, best_anywhere, args.position);

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

    template <unsigned size_>
    auto tbmcsabin(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
    {
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

        // go!
        return max_clique(bit_graph, o, params);
    }
}

auto parasols::tbmcsabin_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 256, "Need to update here if max_graph_size is changed.");
    if (graph.size() < bits_per_word)
        return tbmcsabin<1>(graph, params);
    else if (graph.size() < 2 * bits_per_word)
        return tbmcsabin<2>(graph, params);
    else if (graph.size() < 4 * bits_per_word)
        return tbmcsabin<4>(graph, params);
    else if (graph.size() < 8 * bits_per_word)
        return tbmcsabin<8>(graph, params);
    else if (graph.size() < 16 * bits_per_word)
        return tbmcsabin<16>(graph, params);
    else if (graph.size() < 32 * bits_per_word)
        return tbmcsabin<32>(graph, params);
    else if (graph.size() < 64 * bits_per_word)
        return tbmcsabin<64>(graph, params);
    else if (graph.size() < 128 * bits_per_word)
        return tbmcsabin<128>(graph, params);
    else if (graph.size() < 256 * bits_per_word)
        return tbmcsabin<256>(graph, params);
    else
        throw GraphTooBig();
}


