/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_BMCSA1_MAX_CLIQUE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_BMCSA1_MAX_CLIQUE_HH 1

#include <graph/graph.hh>
#include <max_clique/max_clique_params.hh>

namespace parasols
{
    /**
     * Max clique algorithm.
     *
     * This is San Segundo's BBMCI with a simpler initial vertex ordering, or
     * Prosser's BBMC1.
     */
    auto bmcsa1_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;
}

#endif
