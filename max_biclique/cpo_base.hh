/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_BICLIQUE_CPO_BASE_HH
#define PARASOLS_GUARD_MAX_BICLIQUE_CPO_BASE_HH 1

#include <graph/bit_graph.hh>
#include <max_biclique/max_biclique_params.hh>
#include <max_biclique/max_biclique_result.hh>

namespace parasols
{
    template <unsigned size_, typename VertexType_, typename ActualType_>
    struct CPOBase
    {
        const Graph & original_graph;
        FixedBitGraph<size_> graph;
        const MaxBicliqueParams & params;
        std::vector<int> order;

        auto clique_partition(
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & result) -> void
        {
            FixedBitSet<size_> p_left = p; // not cliqued yet
            int clique = 0;                // current clique
            int i = 0;                     // position in result

            // while we've things left to clique
            while (! p_left.empty()) {
                // next clique
                ++clique;
                // things that can still be given this clique
                FixedBitSet<size_> q = p_left;

                // while we can still give something this clique
                while (! q.empty()) {
                    // first thing we can clique
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything nonadjacent to this the same clique
                    graph.intersect_with_row(v, q);

                    // record in result
                    result[i] = clique;
                    p_order[i] = v;
                    ++i;
                }
            }
        }

        CPOBase(const Graph & g, const MaxBicliqueParams & p) :
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
        }

        template <typename... MoreArgs_>
        auto expand(
                std::vector<unsigned> & ca,
                std::vector<unsigned> & cb,
                FixedBitSet<size_> & pa,
                FixedBitSet<size_> & pb,
                const std::array<VertexType_, size_ * bits_per_word> & pa_order,
                const std::array<VertexType_, size_ * bits_per_word> & pa_bound,
                std::vector<int> & position,
                MoreArgs_ && ... more_args_
                ) -> void
        {
            static_cast<ActualType_ *>(this)->increment_nodes(std::forward<MoreArgs_>(more_args_)...);

            int skip = 0;
            bool keep_going = true;
            static_cast<ActualType_ *>(this)->get_skip(ca.size(), cb.size(), std::forward<MoreArgs_>(more_args_)..., skip, keep_going);

            // for each v in p... (v comes later)
            for (int n = pa.popcount() - 1 ; n >= 0 ; --n) {
                ++position.back();

                // bound, timeout or early exit?
                unsigned best_anywhere_value = static_cast<ActualType_ *>(this)->get_best_anywhere_value();
                if (best_anywhere_value >= params.stop_after_finding || params.abort->load())
                    return;

                if (pa_bound[n] + ca.size() <= best_anywhere_value)
                    return;
                if (pb.popcount() + cb.size() <= best_anywhere_value)
                    return;

                auto v = pa_order[n];

                if (skip > 0) {
                    --skip;
                    pa.unset(v);
                }
                else {
                    // consider taking v
                    ca.push_back(v);
                    pa.unset(v);

                    // filter pb to contain vertices adjacent to v, and pa to contain
                    // vertices not adjacent to v
                    FixedBitSet<size_> new_pa = pa, new_pb = pb;
                    graph.intersect_with_row_complement(v, new_pa);
                    graph.intersect_with_row(v, new_pb);

                    static_cast<ActualType_ *>(this)->potential_new_best(ca, cb, position, std::forward<MoreArgs_>(more_args_)...);

                    if (! new_pb.empty()) {
                        position.push_back(0);
                        std::array<VertexType_, size_ * bits_per_word> new_pb_order;
                        std::array<VertexType_, size_ * bits_per_word> new_pb_bound;
                        clique_partition(new_pb, new_pb_order, new_pb_bound);
                        keep_going = static_cast<ActualType_ *>(this)->recurse(
                                cb, ca, new_pb, new_pa, new_pb_order, new_pb_bound, position, std::forward<MoreArgs_>(more_args_)...) && keep_going;
                        position.pop_back();
                    }

                    // now consider not taking v
                    ca.pop_back();

                    if (! keep_going)
                        break;

                    if (params.break_ab_symmetry) {
                        if (cb.empty())
                            pb.unset(v);
                    }
                }
            }
        }
    };
}

#endif
