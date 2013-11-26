/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/dbmcsa_max_clique.hh>
#include <max_clique/colourise.hh>
#include <max_clique/print_incumbent.hh>
#include <graph/degree_sort.hh>
#include <graph/min_width_sort.hh>
#include <threads/atomic_incumbent.hh>

#include <algorithm>
#include <thread>
#include <mutex>

using namespace parasols;

namespace
{
    template <unsigned size_>
    struct Defer
    {
        std::array<unsigned, size_ * bits_per_word> p_order;
        std::array<unsigned, size_ * bits_per_word> colours;
        FixedBitSet<size_> c;
        FixedBitSet<size_> p;
        std::vector<int> positions;
    };

    template <unsigned size_>
    auto found_possible_new_best(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,
            const FixedBitSet<size_> & c,
            int c_popcount,
            const MaxCliqueParams & params,
            MaxCliqueResult & result,
            AtomicIncumbent & best_anywhere,
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

    template <unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,                      // vertex ordering
            const std::array<unsigned, size_ * bits_per_word> & p_order,
            const std::array<unsigned, size_ * bits_per_word> & colours,
            FixedBitSet<size_> & c,                          // current candidate clique
            FixedBitSet<size_> & p,                          // potential additions
            MaxCliqueResult & result,
            AtomicIncumbent & best_anywhere,
            const MaxCliqueParams & params,
            std::vector<int> & positions,
            bool already_split
            ) -> void
    {
        auto c_popcount = c.popcount();
        int n = p.popcount() - 1;

        // bound, timeout or early exit?
        if (bound(c_popcount, colours[n], params, best_anywhere) || params.abort.load())
            return;

        auto v = p_order[n];
        ++positions.back();

        // consider taking v
        c.set(v);
        ++c_popcount;

        // filter p to contain vertices adjacent to v
        FixedBitSet<size_> new_p = p;
        graph.intersect_with_row(v, new_p);

        if (new_p.empty()) {
            found_possible_new_best(graph, o, c, c_popcount, params, result, best_anywhere, positions);
        }
        else {
            // get our coloured vertices
            ++result.nodes;
            std::array<unsigned, size_ * bits_per_word> new_p_order, new_colours;
            colourise<size_>(graph, new_p, new_p_order, new_colours);
            positions.push_back(0);
            expand<size_>(graph, o, new_p_order, new_colours, c, new_p, result, best_anywhere, params, positions, false);
            positions.pop_back();
        }

        if (! already_split) {
            // now consider not taking v
            c.unset(v);
            p.unset(v);
            --c_popcount;

            if (n > 0) {
                expand<size_>(graph, o, p_order, colours, c, p, result, best_anywhere, params, positions, false);
            }
        }
    }

    template <MaxCliqueOrder order_, unsigned size_>
    auto dbmcsa(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
    {
        MaxCliqueResult result;
        std::mutex result_mutex;
        result.size = params.initial_bound;

        AtomicIncumbent best_anywhere; // global incumbent
        best_anywhere.update(params.initial_bound);

        std::vector<int> o(graph.size()); // vertex ordering

        FixedBitSet<size_> c; // current candidate clique
        c.resize(graph.size());

        FixedBitSet<size_> p; // potential additions
        p.resize(graph.size());
        p.set_all();

        // populate our order with every vertex initially
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

        std::vector<int> positions;
        positions.reserve(graph.size());
        positions.push_back(0);

        // initial ordering
        ++result.nodes;
        std::array<unsigned, size_ * bits_per_word> new_p_order, new_colours;
        colourise<size_>(bit_graph, p, new_p_order, new_colours);

        // first job
        std::mutex next_job_mutex;
        Defer<size_> next_job{ new_p_order, new_colours, c, p, positions };

        std::list<std::thread> threads; // workers
        for (unsigned i = 0 ; i < params.n_threads ; ++i) {
            threads.push_back(std::thread([&, i] {

                auto start_time = std::chrono::steady_clock::now(); // local start time
                MaxCliqueResult tr; // local result

                while (true) {
                    Defer<size_> job;
                    {
                        std::unique_lock<std::mutex> guard(next_job_mutex);
                        if (next_job.p.empty())
                            break;

                        // split
                        job = next_job;

                        ++next_job.positions.back();
                        auto v = next_job.p_order[next_job.p.popcount() - 1];
                        next_job.c.unset(v);
                        next_job.p.unset(v);
                    }

                    expand<size_>(bit_graph, o, job.p_order, job.colours, job.c, job.p, tr, best_anywhere, params, job.positions, true);
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

        for (auto & t : threads)
            t.join();

        return result;
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

