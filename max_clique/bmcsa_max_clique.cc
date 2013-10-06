/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/bmcsa_max_clique.hh>
#include <max_clique/colourise.hh>
#include <max_clique/print_incumbent.hh>
#include <graph/degree_sort.hh>
#include <graph/dkrtj_sort.hh>

#include <algorithm>

using namespace parasols;

namespace
{
    template <MaxCliqueOrder order_, unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,                      // vertex ordering
            FixedBitSet<size_> & c,                          // current candidate clique
            FixedBitSet<size_> & p,                          // potential additions
            MaxCliqueResult & result,
            const MaxCliqueParams & params,
            std::vector<FixedBitSet<size_> > & p_alloc       // pre-allocated space for p
            ) -> void
    {
        ++result.nodes;

        auto c_popcount = c.popcount();

        // get our coloured vertices
        std::array<unsigned, size_ * bits_per_word> p_order, colours;

        // DKRTJ puts more effort into the initial ordering. Don't undo it.
        if (order_ == MaxCliqueOrder::DKRTJ && 0 == c_popcount) {
            for (int i = 0 ; i < graph.size() ; ++i) {
                p_order[i] = i;
                colours[i] = i + 1;
            }
        }
        else
            colourise<size_>(graph, p, p_order, colours);

        // for each v in p... (v comes later)
        for (int n = p.popcount() - 1 ; n >= 0 ; --n) {

            // bound, timeout or early exit?
            if (c_popcount + colours[n] <= result.size || result.size >= params.stop_after_finding || params.abort.load())
                return;

            auto v = p_order[n];

            // consider taking v
            c.set(v);

            // filter p to contain vertices adjacent to v
            auto & new_p = p_alloc[c_popcount];
            new_p = p;
            graph.intersect_with_row(v, new_p);

            if (new_p.empty()) {
                // potential new best
                if (c_popcount + 1 > result.size) {
                    result.size = c_popcount + 1;
                    result.members.clear();
                    for (int i = 0 ; i < graph.size() ; ++i)
                        if (c.test(i))
                            result.members.insert(o[i]);
                    print_incumbent(params, result.size);
                }
            }
            else
                expand<order_, size_>(graph, o, c, new_p, result, params, p_alloc);

            // now consider not taking v
            c.unset(v);
            p.unset(v);
        }
    }

    template <MaxCliqueOrder order_, unsigned size_>
    auto bmcsa(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
    {
        MaxCliqueResult result;
        result.size = params.initial_bound;

        std::vector<int> o(graph.size()); // vertex ordering

        FixedBitSet<size_> c; // current candidate clique
        c.resize(graph.size());

        FixedBitSet<size_> p; // potential additions
        p.resize(graph.size());
        p.set_all();

        std::vector<FixedBitSet<size_> > p_alloc; // space for recursive p values
        p_alloc.resize(graph.size());
        for (auto & a : p_alloc)
            a.resize(graph.size());

        // populate our order with every vertex initially
        std::iota(o.begin(), o.end(), 0);

        switch (order_) {
            case MaxCliqueOrder::Degree:
                degree_sort(graph, o, false);
                break;
            case MaxCliqueOrder::DKRTJ:
                dkrtj_sort(graph, o);
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
        expand<order_, size_>(bit_graph, o, c, p, result, params, p_alloc);

        return result;
    }
}

template <MaxCliqueOrder order_>
auto parasols::bmcsa_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 256, "Need to update here if max_graph_size is changed.");
    if (graph.size() < bits_per_word)
        return bmcsa<order_, 1>(graph, params);
    else if (graph.size() < 2 * bits_per_word)
        return bmcsa<order_, 2>(graph, params);
    else if (graph.size() < 4 * bits_per_word)
        return bmcsa<order_, 4>(graph, params);
    else if (graph.size() < 8 * bits_per_word)
        return bmcsa<order_, 8>(graph, params);
    else if (graph.size() < 16 * bits_per_word)
        return bmcsa<order_, 16>(graph, params);
    else if (graph.size() < 32 * bits_per_word)
        return bmcsa<order_, 32>(graph, params);
    else if (graph.size() < 64 * bits_per_word)
        return bmcsa<order_, 64>(graph, params);
    else if (graph.size() < 128 * bits_per_word)
        return bmcsa<order_, 128>(graph, params);
    else if (graph.size() < 256 * bits_per_word)
        return bmcsa<order_, 256>(graph, params);
    else
        throw GraphTooBig();
}

template auto parasols::bmcsa_max_clique<MaxCliqueOrder::Degree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::bmcsa_max_clique<MaxCliqueOrder::DKRTJ>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

