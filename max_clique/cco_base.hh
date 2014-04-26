/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_CCO_BASE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_CCO_BASE_HH 1

#include <graph/bit_graph.hh>
#include <graph/merge_cliques.hh>

#include <cco/cco.hh>
#include <cco/cco_mixin.hh>

#include <max_clique/max_clique_params.hh>

namespace parasols
{
    enum class CCOInference
    {
        None,
        GlobalDomination,         // just remove from p
        LazyGlobalDomination      // remove from p, lazy
    };

    enum class CCOMerge
    {
        None,
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

    template <CCOInference inference_, unsigned size_, typename VertexType_>
    struct CCOInferer;

    template <unsigned size_, typename VertexType_>
    struct CCOInferer<CCOInference::None, size_, VertexType_>
    {
        void preprocess(FixedBitGraph<size_> &)
        {
        }

        void propagate_no_skip(VertexType_, FixedBitSet<size_> &)
        {
        }

        void propagate_no_immediate(VertexType_, FixedBitSet<size_> &)
        {
        }

        void propagate_no_lazy(VertexType_, FixedBitSet<size_> &)
        {
        }

        auto skip(VertexType_, FixedBitSet<size_> &) -> bool
        {
            return false;
        }
    };

    template <unsigned size_, typename VertexType_>
    struct CCOInferer<CCOInference::GlobalDomination, size_, VertexType_>
    {
        std::vector<FixedBitSet<size_> > unsets;

        void preprocess(FixedBitGraph<size_> & graph)
        {
            unsets.resize(graph.size());
            for (int i = 0 ; i < graph.size() ; ++i)
                unsets[i].resize(graph.size());

            for (int i = 0 ; i < graph.size() ; ++i) {
                for (int j = 0 ; j < graph.size() ; ++j) {
                    if (i == j)
                        continue;

                    FixedBitSet<size_> ni = graph.neighbourhood(i);
                    FixedBitSet<size_> nj = graph.neighbourhood(j);

                    FixedBitSet<size_> nij = ni;
                    nij.intersect_with_complement(nj);
                    nij.unset(j);
                    if (nij.empty())
                        unsets[j].set(i);
                }
            }
        }

        void propagate_no_skip(VertexType_ v, FixedBitSet<size_> & p)
        {
            p.intersect_with_complement(unsets[v]);
        }

        void propagate_no_immediate(VertexType_ v, FixedBitSet<size_> & p)
        {
            p.intersect_with_complement(unsets[v]);
        }

        void propagate_no_lazy(VertexType_, FixedBitSet<size_> &)
        {
        }

        auto skip(VertexType_ v, FixedBitSet<size_> & p) -> bool
        {
            return ! p.test(v);
        }
    };

    template <unsigned size_, typename VertexType_>
    struct CCOInferer<CCOInference::LazyGlobalDomination, size_, VertexType_>
    {
        const FixedBitGraph<size_> * graph;
        std::vector<std::pair<bool, FixedBitSet<size_> > > unsets;

        void preprocess(FixedBitGraph<size_> & g)
        {
            graph = &g;
            unsets.resize(g.size());
        }

        void propagate_no_skip(VertexType_ v, FixedBitSet<size_> & p)
        {
            really_propagate_no(v, p);
        }

        void propagate_no_immediate(VertexType_, FixedBitSet<size_> &)
        {
        }

        void propagate_no_lazy(VertexType_ v, FixedBitSet<size_> & p)
        {
            really_propagate_no(v, p);
        }

        void really_propagate_no(VertexType_ v, FixedBitSet<size_> & p)
        {
            if (! unsets[v].first) {
                unsets[v].first = true;
                unsets[v].second.resize(graph->size());

                FixedBitSet<size_> nv = graph->neighbourhood(v);

                for (int i = 0 ; i < graph->size() ; ++i) {
                    if (i == v)
                        continue;

                    FixedBitSet<size_> niv = graph->neighbourhood(i);
                    niv.intersect_with_complement(nv);
                    niv.unset(v);
                    if (niv.empty())
                        unsets[v].second.set(i);
                }
            }

            p.intersect_with_complement(unsets[v].second);
        }

        auto skip(VertexType_ v, FixedBitSet<size_> & p) -> bool
        {
            return ! p.test(v);
        }
    };

    template <CCOPermutations perm_, CCOInference inference_, CCOMerge merge_,
             unsigned size_, typename VertexType_, typename ActualType_>
    struct CCOBase :
        CCOMixin<size_, VertexType_, CCOBase<perm_, inference_, merge_, size_, VertexType_, ActualType_> >
    {
        using CCOMixin<size_, VertexType_, CCOBase<perm_, inference_, merge_, size_, VertexType_, ActualType_> >::colour_class_order;

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

            inferer.preprocess(graph);
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

            int skip = 0;
            bool keep_going = true;
            static_cast<ActualType_ *>(this)->get_skip(c.size(), std::forward<MoreArgs_>(more_args_)..., skip, keep_going);

            int previous_v = -1;

            // for each v in p... (v comes later)
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

                if (skip > 0 || inferer.skip(v, p)) {
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
                        colour_class_order(SelectColourClassOrderOverload<perm_>(), new_p, new_p_order, new_colours);
                        keep_going = static_cast<ActualType_ *>(this)->recurse(
                                c, new_p, new_p_order, new_colours, position, std::forward<MoreArgs_>(more_args_)...) && keep_going;
                        position.pop_back();
                    }

                    // now consider not taking v
                    c.pop_back();
                    p.unset(v);
                    inferer.propagate_no_immediate(v, p);

                    if (! keep_going)
                        break;
                }
            }
        }
    };
}

#endif
