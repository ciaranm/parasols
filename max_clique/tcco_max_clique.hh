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
    template <CCOPermutations, CCOInference>
    auto tcco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;
}

#endif
