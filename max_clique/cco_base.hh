/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_CCO_BASE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_CCO_BASE_HH 1

#include <graph/bit_graph.hh>

#include <cco/cco.hh>
#include <cco/cco_mixin.hh>

#include <max_clique/max_clique_params.hh>
#include <max_clique/print_incumbent.hh>
#include <max_clique/cco_inference.hh>

#include <numeric>

namespace parasols
{
    enum class CCOMerge
    {
        None,
        Previous,
        All
    };

    template <template <CCOPermutations, CCOInference, CCOMerge, unsigned, typename VertexType_> class WhichCCO_,
             CCOPermutations perm_, CCOInference inference_, CCOMerge merge_>
    struct ApplyPermInferenceMerge
    {
        template <unsigned size_, typename VertexType_> using Type = WhichCCO_<perm_, inference_, merge_, size_, VertexType_>;
    };

    template <template <CCOPermutations, CCOInference, unsigned, typename VertexType_> class WhichCCO_,
             CCOPermutations perm_, CCOInference inference_>
    struct ApplyPermInference
    {
        template <unsigned size_, typename VertexType_> using Type = WhichCCO_<perm_, inference_, size_, VertexType_>;
    };

    template <CCOPermutations perm_, CCOInference inference_, unsigned size_, typename VertexType_, typename ActualType_>
    struct CCOBase :
        CCOMixin<size_, VertexType_, CCOBase<perm_, inference_, size_, VertexType_, ActualType_>, false>
    {
        using CCOMixin<size_, VertexType_, CCOBase<perm_, inference_, size_, VertexType_, ActualType_>, false>::colour_class_order;

        const Graph & original_graph;
        FixedBitGraph<size_> graph;
        const MaxCliqueParams & params;
        std::vector<int> order;

        CCOInferer<inference_, size_, VertexType_> inferer;

        CCOBase(const Graph & g, const MaxCliqueParams & p) :
            original_graph(g),
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

            inferer.preprocess(params, graph);
        }

        template <typename... MoreArgs_>
        auto expand(
                std::vector<unsigned> & c,
                FixedBitSet<size_> & p,
                const std::array<VertexType_, size_ * bits_per_word> & p_order,
                const std::array<VertexType_, size_ * bits_per_word> & colours,
                std::vector<int> & position,
                MoreArgs_ && ... more_args_
                ) -> void
        {
            static_cast<ActualType_ *>(this)->increment_nodes(std::forward<MoreArgs_>(more_args_)...);

            int skip = 0, stop = std::numeric_limits<int>::max();
            bool keep_going = true;
            static_cast<ActualType_ *>(this)->get_skip_and_stop(c.size(), std::forward<MoreArgs_>(more_args_)..., skip, stop, keep_going);

            int previous_v = -1;

            // for each v in p... (v comes later)
            bool first = true;
            for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                ++position.back();

                // bound, timeout or early exit?
                unsigned best_anywhere_value = static_cast<ActualType_ *>(this)->get_best_anywhere_value();
                if (c.size() + colours[n] <= best_anywhere_value || best_anywhere_value >= params.stop_after_finding || params.abort->load())
                    return;

                if (-1 != previous_v)
                    inferer.propagate_no_lazy(previous_v, p);

                auto v = p_order[n];
                previous_v = v;

                if (skip > 0 || inferer.skip(v, p) || (params.vertex_transitive && c.empty() && ! first)) {
                    --skip;
                    p.unset(v);
                    inferer.propagate_no_skip(v, p);
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
                        std::array<VertexType_, size_ * bits_per_word> new_p_order;
                        std::array<VertexType_, size_ * bits_per_word> new_colours;
                        colour_class_order(SelectColourClassOrderOverload<perm_>(), new_p, new_p_order, new_colours, best_anywhere_value - c.size());
                        keep_going = static_cast<ActualType_ *>(this)->recurse(
                                c, new_p, new_p_order, new_colours, position, std::forward<MoreArgs_>(more_args_)...) && keep_going;
                        position.pop_back();
                    }

                    // now consider not taking v
                    c.pop_back();
                    p.unset(v);

                    keep_going = keep_going && (--stop > 0);

                    if (! keep_going)
                        break;
                }

                first = false;
            }
        }
    };
}

#endif
