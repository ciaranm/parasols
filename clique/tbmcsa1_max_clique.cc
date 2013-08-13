/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <clique/tbmcsa1_max_clique.hh>
#include <clique/degree_sort.hh>
#include <clique/colourise.hh>
#include <clique/queue.hh>

#include <boost/thread.hpp>

#include <algorithm>
#include <list>
#include <functional>
#include <vector>
#include <thread>
#include <future>
#include <condition_variable>
#include <atomic>

using namespace clique;

namespace
{
    template <unsigned size_>
    struct QueueItem
    {
        FixedBitSet<size_> c;
        FixedBitSet<size_> p;
        unsigned cn;
    };

    /**
     * Here we only implement using an atomic for the incumbent.
     */
    struct AtomicBestAnywhere
    {
        std::atomic<unsigned> value;

        AtomicBestAnywhere()
        {
            value.store(0, std::memory_order_seq_cst);
        }

        bool update(unsigned v)
        {
            while (true) {
                unsigned cur_v = value.load(std::memory_order_seq_cst);
                if (v > cur_v) {
                    if (value.compare_exchange_strong(cur_v, v, std::memory_order_seq_cst))
                        return true;
                }
                else
                    return false;
            }
        }

        unsigned get()
        {
            return value.load(std::memory_order_relaxed);
        }
    };

    /**
     * We've possibly found a new best. Update best_anywhere and our local
     * result, and do any necessary printing.
     */
    template <unsigned size_>
    auto found_possible_new_best(const FixedBitGraph<size_> & graph, const std::vector<int> & o,
            const FixedBitSet<size_> & c, int c_popcount,
            const MaxCliqueParams & params, MaxCliqueResult & result, AtomicBestAnywhere & best_anywhere) -> void
    {
        if (best_anywhere.update(c_popcount)) {
            result.size = c_popcount;
            result.members.clear();
            for (int i = 0 ; i < graph.size() ; ++i)
                if (c.test(i))
                    result.members.insert(o[i]);
            if (params.print_candidates)
                print_candidate(params, result.size);
        }
    }

    /**
     * Bound function.
     */
    auto bound(unsigned c_popcount, unsigned cn, const MaxCliqueParams & params, AtomicBestAnywhere & best_anywhere) -> bool
    {
        unsigned best_anywhere_value = best_anywhere.get();
        return (c_popcount + cn <= best_anywhere_value || best_anywhere_value >= params.stop_after_finding);
    }

    template <unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,                      // vertex ordering
            Queue<QueueItem<size_> > * const maybe_queue,    // not null if we're populating: enqueue here
            Queue<QueueItem<size_> > * const donation_queue, // not null if we're donating: donate here
            FixedBitSet<size_> & c,                          // current candidate clique
            FixedBitSet<size_> & p,                          // potential additions
            MaxCliqueResult & result,
            const MaxCliqueParams & params,
            std::vector<FixedBitSet<size_> > & p_alloc,      // pre-allocated space for p
            AtomicBestAnywhere & best_anywhere) -> void
    {
        ++result.nodes;

        // get our coloured vertices
        std::array<unsigned, size_ * bits_per_word> p_order, colours;
        colourise<size_>(graph, p, p_order, colours);

        auto c_popcount = c.popcount();

        bool chose_to_donate = false;

        // for each v in p... (v comes later)
        for (int n = p.popcount() - 1 ; n >= 0 ; --n) {

            // bound, timeout or early exit?
            if (bound(c_popcount, colours[n], params, best_anywhere) || params.abort.load())
                return;

            auto v = p_order[n];

            // consider taking v
            c.set(v);

            // filter p to contain vertices adjacent to v
            auto & new_p = p_alloc[c_popcount];
            new_p = p;
            graph.intersect_with_row(v, new_p);

            if (new_p.empty()) {
                found_possible_new_best(graph, o, c, c_popcount + 1, params, result, best_anywhere);
            }
            else
            {
                // do we enqueue or recurse?
                bool should_expand = true;

                if (maybe_queue && c_popcount + 1 == params.split_depth) {
                    maybe_queue->enqueue_blocking(QueueItem<size_>{ c, std::move(new_p), colours[n] }, params.n_threads);
                    should_expand = false;
                }
                else if (donation_queue && (chose_to_donate || donation_queue->want_donations())) {
                    donation_queue->enqueue(QueueItem<size_>{ c, std::move(new_p), colours[n] });
                    should_expand = false;
                    chose_to_donate = true;
                    ++result.donations;
                }

                if (should_expand)
                    expand<size_>(graph, o, maybe_queue, donation_queue, c, new_p, result, params, p_alloc, best_anywhere);
            }

            // now consider not taking v
            c.unset(v);
            p.unset(v);
        }
    }

    template <unsigned size_>
    auto max_clique(const FixedBitGraph<size_> & graph, const std::vector<int> & o, const MaxCliqueParams & params) -> MaxCliqueResult
    {
        Queue<QueueItem<size_> > queue{ params.n_threads, params.work_donation }; // work queue

        MaxCliqueResult result; // global result
        std::mutex result_mutex;

        AtomicBestAnywhere best_anywhere; // global incumbent
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

                    std::vector<FixedBitSet<size_> > p_alloc;
                    p_alloc.resize(graph.size());
                    for (auto & a : p_alloc)
                        a.resize(graph.size());

                    // populate!
                    expand<size_>(graph, o, &queue, nullptr, tc, tp, result, params, p_alloc, best_anywhere);

                    // merge results
                    queue.initial_producer_done();
                    std::unique_lock<std::mutex> guard(result_mutex);
                    result.merge(tr);
                    }));

        /* workers */
        for (unsigned i = 0 ; i < params.n_threads ; ++i) {
            threads.push_back(std::thread([&, i] {
                        auto start_time = std::chrono::steady_clock::now(); // local start time

                        MaxCliqueResult tr; // local result

                        std::vector<FixedBitSet<size_> > p_alloc;
                        p_alloc.resize(graph.size());
                        for (auto & a : p_alloc)
                            a.resize(graph.size());

                        while (true) {
                            // get some work to do
                            QueueItem<size_> args;
                            if (! queue.dequeue_blocking(args))
                                break;

                            // re-evaluate the bound against our new best
                            if (args.cn <= best_anywhere.get())
                                continue;

                            // do some work
                            expand<size_>(graph, o, nullptr, params.work_donation ? &queue : nullptr,
                                    args.c, args.p, tr, params, p_alloc, best_anywhere);

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
    auto tbmcsa1(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
    {
        std::vector<int> o(graph.size()); // vertex ordering
        std::iota(o.begin(), o.end(), 0);
        degree_sort(graph, o);

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

auto clique::tbmcsa1_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 256, "Need to update here if max_graph_size is changed.");
    if (graph.size() < bits_per_word)
        return tbmcsa1<1>(graph, params);
    else if (graph.size() < 2 * bits_per_word)
        return tbmcsa1<2>(graph, params);
    else if (graph.size() < 4 * bits_per_word)
        return tbmcsa1<4>(graph, params);
    else if (graph.size() < 8 * bits_per_word)
        return tbmcsa1<8>(graph, params);
    else if (graph.size() < 16 * bits_per_word)
        return tbmcsa1<16>(graph, params);
    else if (graph.size() < 32 * bits_per_word)
        return tbmcsa1<32>(graph, params);
    else if (graph.size() < 64 * bits_per_word)
        return tbmcsa1<64>(graph, params);
    else if (graph.size() < 128 * bits_per_word)
        return tbmcsa1<128>(graph, params);
    else if (graph.size() < 256 * bits_per_word)
        return tbmcsa1<256>(graph, params);
    else
        throw GraphTooBig();
}

