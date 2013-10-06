/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_TBMCSA_MAX_CLIQUE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_TBMCSA_MAX_CLIQUE_HH 1

#include <graph/graph.hh>
#include <max_clique/max_clique_params.hh>
#include <max_clique/max_clique_result.hh>

namespace parasols
{
    /**
     * Max clique algorithm.
     *
     * This is our threaded version of San Segundo's BBMCI with a selectable
     * initial vertex ordering, or Prosser's BBMC1 for some value of 1.
     */
    template <MaxCliqueOrder order_>
    auto tbmcsa_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;
}

#endif
