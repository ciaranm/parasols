/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_CCO_BASE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_CCO_BASE_HH 1

#include <graph/bit_graph.hh>

#include <cco/cco.hh>
#include <cco/cco_mixin.hh>

#include <max_clique/max_clique_params.hh>

namespace parasols
{
    template <CCOPermutations perm_, unsigned size_, typename VertexType_, typename ActualType_>
    struct CCOBase :
        CCOMixin<size_, VertexType_, CCOBase<perm_, size_, VertexType_, ActualType_> >
    {
        using CCOMixin<size_, VertexType_, CCOBase<perm_, size_, VertexType_, ActualType_> >::colour_class_order;

        FixedBitGraph<size_> graph;
        const MaxCliqueParams & params;
        std::vector<int> order;

        CCOBase(const Graph & g, const MaxCliqueParams & p) :
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

        template <typename... MoreArgs_>
        auto expand(
                std::vector<unsigned> & c,                       // current candidate clique
                FixedBitSet<size_> & p,                          // potential additions
                std::vector<int> & position,
                MoreArgs_ && ... more_args_
                ) -> void
        {
            static_cast<ActualType_ *>(this)->increment_nodes(std::forward<MoreArgs_>(more_args_)...);

            int skip = 0;
            bool keep_going = true;
            static_cast<ActualType_ *>(this)->get_skip(c.size(), std::forward<MoreArgs_>(more_args_)..., skip, keep_going);

            // get our coloured vertices
            std::array<VertexType_, size_ * bits_per_word> p_order, colours;
            colour_class_order(SelectColourClassOrderOverload<perm_>(), p, p_order, colours);

            // for each v in p... (v comes later)
            for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                ++position.back();

                // bound, timeout or early exit?
                unsigned best_anywhere_value = static_cast<ActualType_ *>(this)->get_best_anywhere_value();
                if (c.size() + colours[n] <= best_anywhere_value || best_anywhere_value >= params.stop_after_finding || params.abort.load())
                    return;

                auto v = p_order[n];

                if (skip > 0) {
                    --skip;
                    p.unset(v);
                }
                else {
                    // consider taking v
                    c.push_back(v);

                    // filter p to contain vertices adjacent to v
                    FixedBitSet<size_> new_p = p;
                    graph.intersect_with_row(v, new_p);

                    if (new_p.empty()) {
                        static_cast<ActualType_ *>(this)->potential_new_best(c, position, std::forward<MoreArgs_>(more_args_)...);
                    }
                    else {
                        position.push_back(0);
                        keep_going = static_cast<ActualType_ *>(this)->recurse(c, new_p, position, std::forward<MoreArgs_>(more_args_)...) && keep_going;
                        position.pop_back();
                    }

                    // now consider not taking v
                    c.pop_back();
                    p.unset(v);

                    if (! keep_going)
                        break;
                }
            }
        }
    };
}

#endif
