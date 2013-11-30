/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/tbmcsa_max_clique.hh>
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

    auto waited_long_enough(
            const MaxCliqueParams & params,
            std::chrono::time_point<std::chrono::steady_clock> & last_donation_time) -> bool
    {
        if (0 == params.donation_wait)
            return true;

        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(now - last_donation_time);
        if (delta.count() > params.donation_wait) {
            last_donation_time = now;
            return true;
        }
        else
            return false;
    }

    template <MaxCliqueOrder order_, unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,                      // vertex ordering
            Queue<QueueItem<size_> > * const maybe_queue,    // not null if we're populating: enqueue here
            Queue<QueueItem<size_> > * const donation_queue, // not null if we're donating: donate here
            std::chrono::time_point<std::chrono::steady_clock> & last_donation_time,
            FixedBitSet<size_> & c,                          // current candidate clique
            FixedBitSet<size_> & p,                          // potential additions
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

        bool chose_to_donate = false;

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

            // filter p to contain vertices adjacent to v
            FixedBitSet<size_> new_p = p;
            new_p = p;
            graph.intersect_with_row(v, new_p);

            if (new_p.empty()) {
                found_possible_new_best(graph, o, c, c_popcount, params, result, best_anywhere, position);
            }
            else
            {
                // do we enqueue or recurse?
                bool should_expand = true;

                if (maybe_queue && c_popcount == params.split_depth) {
                    auto new_position = position;
                    new_position.push_back(0);
                    maybe_queue->enqueue_blocking(QueueItem<size_>{ c, std::move(new_p), c_popcount + colours[n], std::move(new_position) }, params.n_threads);
                    should_expand = false;
                }
                else if (new_p.popcount() < params.min_donate_size) {
                    chose_to_donate = false;
                }
                else if (donation_queue && (chose_to_donate ||
                            (donation_queue->want_donations() && waited_long_enough(params, last_donation_time)))) {
                    auto new_position = position;
                    new_position.push_back(0);
                    donation_queue->enqueue(QueueItem<size_>{ c, std::move(new_p), c_popcount + colours[n], std::move(new_position) });
                    should_expand = false;
                    chose_to_donate = true;
                    ++result.donations;
                }

                if (should_expand) {
                    position.push_back(0);
                    expand<order_, size_>(graph, o, maybe_queue, donation_queue, last_donation_time, c, new_p, result, params, best_anywhere, position);
                    position.pop_back();
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
        Queue<QueueItem<size_> > queue{ params.n_threads, params.work_donation, params.donate_when_empty }; // work queue

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

                    auto last_donation_time = std::chrono::steady_clock::now();

                    // populate!
                    expand<order_, size_>(graph, o, &queue, nullptr, last_donation_time, tc, tp, result, params, best_anywhere, position);

                    // merge results
                    queue.initial_producer_done();
                    std::unique_lock<std::mutex> guard(result_mutex);
                    result.merge(tr);
                    }));

        /* workers */
        for (unsigned i = 0 ; i < params.n_threads ; ++i) {
            threads.push_back(std::thread([&, i] {
                        auto start_time = std::chrono::steady_clock::now(); // local start time
                        auto last_donation_time = std::chrono::steady_clock::now();

                        MaxCliqueResult tr; // local result

                        while (true) {
                            // get some work to do
                            QueueItem<size_> args;
                            if (! queue.dequeue_blocking(args))
                                break;

                            // re-evaluate the bound against our new best
                            if (args.cn <= best_anywhere.get())
                                continue;

                            // do some work
                            expand<order_, size_>(graph, o, nullptr, params.work_donation ? &queue : nullptr, last_donation_time,
                                    args.c, args.p, tr, params, best_anywhere, args.position);
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
    auto tbmcsa(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
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
auto parasols::tbmcsa_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 1024, "Need to update here if max_graph_size is changed.");
    if (graph.size() < bits_per_word)
        return tbmcsa<order_, 1>(graph, params);
    else if (graph.size() < 2 * bits_per_word)
        return tbmcsa<order_, 2>(graph, params);
    else if (graph.size() < 4 * bits_per_word)
        return tbmcsa<order_, 4>(graph, params);
    else if (graph.size() < 8 * bits_per_word)
        return tbmcsa<order_, 8>(graph, params);
    else if (graph.size() < 16 * bits_per_word)
        return tbmcsa<order_, 16>(graph, params);
    else if (graph.size() < 32 * bits_per_word)
        return tbmcsa<order_, 32>(graph, params);
    else if (graph.size() < 64 * bits_per_word)
        return tbmcsa<order_, 64>(graph, params);
    else if (graph.size() < 128 * bits_per_word)
        return tbmcsa<order_, 128>(graph, params);
    else if (graph.size() < 256 * bits_per_word)
        return tbmcsa<order_, 256>(graph, params);
    else if (graph.size() < 512 * bits_per_word)
        return tbmcsa<order_, 512>(graph, params);
    else if (graph.size() < 1024 * bits_per_word)
        return tbmcsa<order_, 1024>(graph, params);
    else
        throw GraphTooBig();
}

template auto parasols::tbmcsa_max_clique<MaxCliqueOrder::Degree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::tbmcsa_max_clique<MaxCliqueOrder::MinWidth>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::tbmcsa_max_clique<MaxCliqueOrder::ExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::tbmcsa_max_clique<MaxCliqueOrder::DynExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

