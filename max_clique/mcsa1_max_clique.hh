/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_MCSA1_MAX_CLIQUE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_MCSA1_MAX_CLIQUE_HH 1

#include <graph/graph.hh>
#include <max_clique/max_clique_params.hh>
#include <max_clique/max_clique_result.hh>

namespace parasols
{
    /**
     * Max clique algorithm.
     *
     * This is Tomita's MCS with non-increasing degree ordering and no colour
     * repair step, or Prosser's MCSa1.
     */
    auto mcsa1_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;
}

#endif
