/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/ost_max_clique.hh>
#include <max_clique/print_incumbent.hh>
#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>
#include <numeric>

using namespace parasols;

namespace
{
    template <unsigned size_, typename VertexType_>
    struct OST
    {
        MaxCliqueResult result;
        FixedBitGraph<size_> graph;
        const MaxCliqueParams & params;
        std::vector<int> order;
        std::vector<int> subproblems;

        OST(const Graph & g, const MaxCliqueParams & p) :
            params(p),
            order(g.size()),
            subproblems(g.size())
        {
            // populate our order with every vertex initially
            std::iota(order.begin(), order.end(), 0);
            params.order_function(g, order);

            // re-encode graph as a bit graph
            graph.resize(g.size());

            for (int i = 0 ; i < g.size() ; ++i)
                for (int j = 0 ; j < g.size() ; ++j)
                    if (g.adjacent(order[i], order[j]))
                        graph.add_edge(i, j);
        }

        auto expand(
                std::vector<unsigned> & c,
                FixedBitSet<size_> & p,
                std::vector<int> & position,
                bool & found
                ) -> void
        {
            ++result.nodes;

            while (! p.empty()) {
                ++position.back();

                auto v = p.first_set_bit();

                // bound, timeout or early exit?
                if (c.size() + p.popcount() <= result.size || c.size() + subproblems[v] <= result.size ||
                        result.size >= params.stop_after_finding || params.abort->load())
                    return;

                // consider taking v
                c.push_back(v);

                // filter p to contain vertices adjacent to v
                FixedBitSet<size_> new_p = p;
                graph.intersect_with_row(v, new_p);

                if (new_p.empty()) {
                    if (c.size() > result.size) {
                        result.size = c.size();

                        result.members.clear();
                        for (auto & v : c)
                            result.members.insert(order[v]);

                        print_incumbent(params, c.size(), position);

                        found = true;
                    }
                }
                else {
                    position.push_back(0);
                    expand(c, new_p, position, found);
                    position.pop_back();
                }

                // now consider not taking v
                c.pop_back();
                p.unset(v);

                if (found)
                    break;
            }
        }

        auto run() -> MaxCliqueResult
        {
            result.size = params.initial_bound;

            subproblems[graph.size() - 1] = 1;

            for (int i = graph.size() - 1 ; i >= 0 ; --i) {
                std::vector<unsigned> c;
                c.reserve(graph.size());

                FixedBitSet<size_> p; // potential additions
                for (int j = i ; j < graph.size() ; ++j)
                    p.set(j);

                std::vector<int> positions;
                positions.reserve(graph.size());
                positions.push_back(i);
                positions.push_back(0);

                bool found = false;
                expand(c, p, positions, found);

                positions.pop_back();

                print_position(params, "subproblem is " + std::to_string(result.size), positions);
                subproblems[i] = result.size;
                if (0 != i)
                    subproblems[i - 1] = subproblems[i] + 1;
            }

            return result;
        }
    };
}

auto parasols::ost_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<OST, MaxCliqueResult>(AllGraphSizes(), graph, params);
}

