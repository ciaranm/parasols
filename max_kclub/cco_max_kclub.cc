/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_kclub/cco_max_kclub.hh>
#include <max_kclub/print_incumbent.hh>
#include <max_kclub/kneighbours.hh>

#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>

#include <algorithm>
#include <thread>
#include <mutex>

using namespace parasols;

namespace
{
    template <unsigned size_, typename VertexType_>
    struct CCO
    {
        const Graph & orig_graph;
        FixedBitGraph<size_> graph;
        const MaxKClubParams & params;
        std::vector<int> order;
        MaxKClubResult result;

        CCO(const Graph & g, const MaxKClubParams & p) :
            orig_graph(g),
            params(p),
            order(g.size())
        {
            // populate our order with every vertex initially
            std::iota(order.begin(), order.end(), 0);
            params.order_function(g, order);

            // re-encode to get g^k as a bit graph
            KNeighbours kneighbours(g, params);

            graph.resize(g.size());
            for (int i = 0 ; i < g.size() ; ++i)
                for (int j = 0 ; j < g.size() ; ++j)
                    if (kneighbours.vertices[order[i]].distances[order[j]].distance > 0)
                        graph.add_edge(i, j);
        }

        auto expand(
                FixedBitSet<size_> & c,                          // current candidate clique
                FixedBitSet<size_> & p,                          // potential additions
                std::vector<int> & position
                ) -> void
        {
            ++result.nodes;

            auto c_popcount = c.popcount();

            // get our coloured vertices
            std::array<VertexType_, size_ * bits_per_word> p_order, colours;
            colour_class_order(p, p_order, colours);

            // for each v in p... (v comes later)
            for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                ++position.back();

                // bound, timeout or early exit?
                if (c_popcount + colours[n] <= result.size || result.size >= params.stop_after_finding || params.abort->load())
                    return;

                auto v = p_order[n];

                // consider taking v
                c.set(v);
                ++c_popcount;

                potential_new_best(c_popcount, c, position);

                // filter p to contain vertices adjacent to v
                FixedBitSet<size_> new_p = p;
                graph.intersect_with_row(v, new_p);

                if (! new_p.empty()) {
                    position.push_back(0);
                    expand(c, new_p, position);
                    position.pop_back();
                }

                // now consider not taking v
                c.unset(v);
                p.unset(v);
                --c_popcount;
            }
        }

        auto colour_class_order(
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            VertexType_ colour = 0;        // current colour
            VertexType_ i = 0;             // position in p_bounds

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<size_> q = p_left;

                // while we can still give something this colour
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    graph.intersect_with_row_complement(v, q);

                    // record in result
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                }

            }
        }

        auto run() -> MaxKClubResult
        {
            result.size = params.initial_bound;

            FixedBitSet<size_> c; // current candidate clique
            c.resize(graph.size());

            FixedBitSet<size_> p; // potential additions
            p.resize(graph.size());
            p.set_all();

            std::vector<int> positions;
            positions.reserve(graph.size());
            positions.push_back(0);

            // go!
            expand(c, p, positions);

            return result;
        }

        auto potential_new_best(
                unsigned c_popcount,
                const FixedBitSet<size_> & c,
                std::vector<int> & position) -> void
        {
            // potential new best
            if (c_popcount > result.size) {
                std::vector<int> restrict_to((graph.size()));
                for (int i = 0 ; i < graph.size() ; ++i)
                    if (c.test(i))
                        restrict_to[order[i]] = 1;

                KNeighbours kneighbours(orig_graph, params, &restrict_to);

                bool is_complete = true;
                for (int i = 0 ; i < graph.size() && is_complete ; ++i)
                    if (c.test(i))
                        for (int j = 0 ; j < graph.size() && is_complete ; ++j)
                            if (i != j && c.test(j))
                                if (! (kneighbours.vertices[order[i]].distances[order[j]].distance > 0))
                                    is_complete = false;

                if (is_complete) {
                    unsigned old_size = result.size;
                    result.size = c_popcount;
                    result.members.clear();
                    for (int i = 0 ; i < graph.size() ; ++i)
                        if (c.test(i))
                            result.members.insert(order[i]);

                    print_incumbent(params, c_popcount, old_size, true, position);
                }
                else
                    print_incumbent(params, c_popcount, result.size, false, position);
            }
        }
    };
}

auto parasols::cco_max_kclub(const Graph & graph, const MaxKClubParams & params) -> MaxKClubResult
{
    return select_graph_size<CCO, MaxKClubResult>(AllGraphSizes(), graph, params);
}

