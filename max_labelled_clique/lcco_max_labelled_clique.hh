/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_LABELLED_CLIQUE_LCCO_MAX_LABELLED_CLIQUE_HH
#define PARASOLS_GUARD_MAX_LABELLED_CLIQUE_LCCO_MAX_LABELLED_CLIQUE_HH 1

#include <graph/graph.hh>
#include <cco/cco.hh>
#include <max_labelled_clique/max_labelled_clique_params.hh>
#include <max_labelled_clique/max_labelled_clique_result.hh>

namespace parasols
{
    /**
     * Max labelled clique algorithm which is mostly just a max clique algorithm
     * with some extra feasibility checks.
     */
    template <CCOPermutations perm_>
    auto lcco_max_labelled_clique(const Graph & graph, const MaxLabelledCliqueParams &) -> MaxLabelledCliqueResult;
}

#endif
