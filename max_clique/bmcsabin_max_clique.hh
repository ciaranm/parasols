/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_BMCSABIN_MAX_CLIQUE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_BMCSABIN_MAX_CLIQUE_HH 1

#include <graph/graph.hh>
#include <max_clique/max_clique_params.hh>
#include <max_clique/max_clique_result.hh>

namespace parasols
{
    /**
     * Max clique algorithm.
     *
     * Like bmcsa, but refactored to be a binary / binomial search.
     */
    auto bmcsabin_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;
}

#endif
