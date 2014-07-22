/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_CCO_INFERENCE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_CCO_INFERENCE_HH 1

#include <max_clique/max_clique_params.hh>
#include <max_clique/print_incumbent.hh>

#include <graph/bit_graph.hh>

namespace parasols
{
    enum class CCOInference
    {
        None,
        LazyGlobalDomination      // remove from p, lazy
    };

    template <CCOInference inference_, unsigned size_, typename VertexType_>
    struct CCOInferer;

    template <unsigned size_, typename VertexType_>
    struct CCOInferer<CCOInference::None, size_, VertexType_>
    {
        void preprocess(const MaxCliqueParams &, FixedBitGraph<size_> &)
        {
        }

        void propagate_no_skip(VertexType_, FixedBitSet<size_> &)
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
    struct CCOInferer<CCOInference::LazyGlobalDomination, size_, VertexType_>
    {
        const FixedBitGraph<size_> * graph;
        std::vector<std::pair<bool, FixedBitSet<size_> > > unsets;

        void preprocess(const MaxCliqueParams &, FixedBitGraph<size_> & g)
        {
            graph = &g;
            unsets.resize(g.size());
        }

        void propagate_no_skip(VertexType_ v, FixedBitSet<size_> & p)
        {
            really_propagate_no(v, p);
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
}

#endif
