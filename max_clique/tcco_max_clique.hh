/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_TCCO_MAX_CLIQUE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_TCCO_MAX_CLIQUE_HH 1

#include <graph/graph.hh>
#include <cco/cco.hh>
#include <max_clique/cco_base.hh>
#include <max_clique/max_clique_params.hh>
#include <max_clique/max_clique_result.hh>

namespace parasols
{
    /**
     * Super duper max clique algorithm, threaded.
     */
    template <CCOPermutations, CCOInference, bool merge_queue_>
    auto tcco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;

    template <template <CCOPermutations, CCOInference, bool, unsigned, typename VertexType_> class WhichCCO_,
             CCOPermutations perm_, CCOInference inference_, bool merge_queue_>
    struct ApplyPermInferenceMQ
    {
        template <unsigned size_, typename VertexType_> using Type = WhichCCO_<perm_, inference_, merge_queue_, size_, VertexType_>;
    };
}

#endif
