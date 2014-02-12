/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/bmcsa_max_clique.hh>
#include <max_clique/colourise.hh>
#include <max_clique/print_incumbent.hh>

#include <graph/is_club.hh>

#include <algorithm>

using namespace parasols;

namespace
{
    template <unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,                      // vertex ordering
            FixedBitSet<size_> & c,                          // current candidate clique
            FixedBitSet<size_> & p,                          // potential additions
            MaxCliqueResult & result,
            const MaxCliqueParams & params,
            std::vector<int> & position
            ) -> void
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
            if (c_popcount + colours[n] <= result.size || result.size >= params.stop_after_finding || params.abort.load())
                return;

            auto v = p_order[n];

            // consider taking v
            c.set(v);
            ++c_popcount;

            // filter p to contain vertices adjacent to v
            FixedBitSet<size_> new_p = p;
            graph.intersect_with_row(v, new_p);

            if (new_p.empty()) {
                // potential new best
                if (c_popcount > result.size) {
                    if (params.enumerate) {
                        ++result.result_count;
                    }
                    else
                        result.size = c_popcount;

                    result.members.clear();
                    for (int i = 0 ; i < graph.size() ; ++i)
                        if (c.test(i))
                            result.members.insert(o[i]);

                    if (params.check_clubs) {
                        bool club = is_club(*params.original_graph, params.power, result.members);
                        if (params.enumerate && club)
                            ++result.result_club_count;
                        print_incumbent(params, c_popcount, position, club);
                    }
                    else
                        print_incumbent(params, c_popcount, position);
                }
            }
            else {
                position.push_back(0);
                expand<size_>(graph, o, c, new_p, result, params, position);
                position.pop_back();
            }

            // now consider not taking v
            c.unset(v);
            p.unset(v);
            --c_popcount;
        }
    }

    template <unsigned size_>
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

        // populate our order with every vertex initially
        std::iota(o.begin(), o.end(), 0);
        params.order_function(graph, o);

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

        // go!
        expand<size_>(bit_graph, o, c, p, result, params, positions);

        // hack for enumerate
        if (params.enumerate)
            result.size = result.members.size();

        return result;
    }
}

auto parasols::bmcsa_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 1024, "Need to update here if max_graph_size is changed.");
    if (graph.size() < bits_per_word)
        return bmcsa<1>(graph, params);
    else if (graph.size() < 2 * bits_per_word)
        return bmcsa<2>(graph, params);
    else if (graph.size() < 4 * bits_per_word)
        return bmcsa<4>(graph, params);
    else if (graph.size() < 8 * bits_per_word)
        return bmcsa<8>(graph, params);
    else if (graph.size() < 16 * bits_per_word)
        return bmcsa<16>(graph, params);
    else if (graph.size() < 32 * bits_per_word)
        return bmcsa<32>(graph, params);
    else if (graph.size() < 64 * bits_per_word)
        return bmcsa<64>(graph, params);
    else if (graph.size() < 128 * bits_per_word)
        return bmcsa<128>(graph, params);
    else if (graph.size() < 256 * bits_per_word)
        return bmcsa<256>(graph, params);
    else if (graph.size() < 512 * bits_per_word)
        return bmcsa<512>(graph, params);
    else if (graph.size() < 1024 * bits_per_word)
        return bmcsa<1024>(graph, params);
    else
        throw GraphTooBig();
}

