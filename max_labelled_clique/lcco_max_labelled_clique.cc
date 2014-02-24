/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_labelled_clique/lcco_max_labelled_clique.hh>
#include <max_labelled_clique/print_incumbent.hh>

#include <cco/cco_mixin.hh>
#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>

using namespace parasols;

namespace
{
    using LabelSet = FixedBitSet<2>;

    template <CCOPermutations perm_, unsigned size_>
    struct LCCO :
        CCOMixin<size_, LCCO<perm_, size_> >
    {
        using CCOMixin<size_, LCCO<perm_, size_> >::colour_class_order;

        MaxLabelledCliqueResult result;
        FixedBitGraph<size_> graph;
        const MaxLabelledCliqueParams & params;
        std::vector<int> order;
        bool pass_2;

        LCCO(const Graph & g, const MaxLabelledCliqueParams & p) :
            params(p),
            order(g.size())
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
                FixedBitSet<size_> & c,                          // current candidate clique
                FixedBitSet<size_> & p,                          // potential additions
                LabelSet & u,
                std::vector<int> & position
                ) -> void
        {
            ++result.nodes;

            auto c_popcount = c.popcount();

            // get our coloured vertices
            std::array<unsigned, size_ * bits_per_word> p_order, colours;
            colour_class_order(SelectColourClassOrderOverload<perm_>(), p, p_order, colours);

            // for each v in p... (v comes later)
            for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                ++position.back();

                // bound, timeout or early exit?
                unsigned best_anywhere_value = pass_2 ? result.size - 1 : result.size;
                if (c_popcount + colours[n] <= best_anywhere_value || best_anywhere_value >= params.stop_after_finding || params.abort.load())
                    return;

                auto v = p_order[n];

                // consider taking v
                c.set(v);
                ++c_popcount;

                // filter p to contain vertices adjacent to v
                FixedBitSet<size_> new_p = p;
                graph.intersect_with_row(v, new_p);

                // used new label?
                LabelSet new_u = u;
                for (int i = 0 ; i < graph.size() ; ++i)
                    if (c.test(i) && graph.adjacent(v, i))
                        new_u.set(params.labels.at(v).at(i));

                unsigned new_u_popcount = new_u.popcount();

                if (new_u_popcount <= params.budget) {
                    potential_new_best(c_popcount, c, new_u_popcount, position);

                    if ((! pass_2) || (new_u_popcount < result.cost)) {
                        if (! new_p.empty()) {
                            position.push_back(0);
                            expand(c, new_p, new_u, position);
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

        auto run() -> MaxLabelledCliqueResult
        {
            result.size = params.initial_bound;

            for (unsigned pass = 1 ; pass <= 2 ; ++pass) {
                pass_2 = pass == 2;

                FixedBitSet<size_> c; // current candidate clique
                c.resize(graph.size());

                FixedBitSet<size_> p; // potential additions
                p.resize(graph.size());
                p.set_all();

                std::vector<int> positions;
                positions.reserve(graph.size());
                positions.push_back(0);

                LabelSet u;

                // go!
                expand(c, p, u, positions);
            }

            return result;
        }

        auto potential_new_best(
                unsigned c_popcount,
                const FixedBitSet<size_> & c,
                unsigned cost,
                std::vector<int> & position) -> void
        {
            // potential new best
            if (c_popcount > result.size || (c_popcount == result.size && cost < result.cost)) {
                result.size = c_popcount;
                result.cost = cost;

                result.members.clear();
                for (int i = 0 ; i < graph.size() ; ++i)
                    if (c.test(i))
                        result.members.insert(order[i]);

                print_incumbent(params, c_popcount, cost, position);
            }
        }
    };
}

template <CCOPermutations perm_>
auto parasols::lcco_max_labelled_clique(const Graph & graph, const MaxLabelledCliqueParams & params) -> MaxLabelledCliqueResult
{
    return select_graph_size<ApplyPerm<LCCO, perm_>::template Type, MaxLabelledCliqueResult>(AllGraphSizes(), graph, params);
}

template auto parasols::lcco_max_labelled_clique<CCOPermutations::None>(const Graph &, const MaxLabelledCliqueParams &) -> MaxLabelledCliqueResult;
template auto parasols::lcco_max_labelled_clique<CCOPermutations::Defer1>(const Graph &, const MaxLabelledCliqueParams &) -> MaxLabelledCliqueResult;
template auto parasols::lcco_max_labelled_clique<CCOPermutations::Sort>(const Graph &, const MaxLabelledCliqueParams &) -> MaxLabelledCliqueResult;

