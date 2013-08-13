/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_TMCSA1_MAX_CLIQUE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_TMCSA1_MAX_CLIQUE_HH 1

#include <graph/graph.hh>
#include <max_clique/max_clique_params.hh>

namespace parasols
{
    /**
     * Threaded max clique algorithm.
     *
     * This is our threaded version of Tomita's MCS with non-increasing degree
     * ordering and no colour repair step, or Prosser's MCSa1.
     */
    auto tmcsa1_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;
}

#endif
