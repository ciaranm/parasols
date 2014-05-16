/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_BICLIQUE_CPO_MAX_BICLIQUE_HH
#define PARASOLS_GUARD_MAX_BICLIQUE_CPO_MAX_BICLIQUE_HH 1

#include <graph/graph.hh>
#include <max_biclique/cpo_base.hh>
#include <max_biclique/max_biclique_params.hh>
#include <max_biclique/max_biclique_result.hh>

namespace parasols
{
    auto cpo_max_biclique(const Graph & graph, const MaxBicliqueParams & params) -> MaxBicliqueResult;
}

#endif
