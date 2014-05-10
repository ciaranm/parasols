/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_CCO_MAX_CLIQUE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_CCO_MAX_CLIQUE_HH 1

#include <graph/graph.hh>
#include <cco/cco.hh>
#include <max_clique/cco_base.hh>
#include <max_clique/max_clique_params.hh>
#include <max_clique/max_clique_result.hh>

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

    /**
     * Super duper max clique algorithm.
     */
    template <CCOPermutations, CCOInference, CCOMerge>
    auto cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;
}

#endif
