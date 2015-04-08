/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_TVB_SUBGRAPH_ISOMORPHISM_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_TVB_SUBGRAPH_ISOMORPHISM_HH 1

#include <subgraph_isomorphism/subgraph_isomorphism_params.hh>
#include <subgraph_isomorphism/subgraph_isomorphism_result.hh>
#include <graph/graph.hh>

namespace parasols
{
    auto ttvb_dpd_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto ttvbbj_dpd_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto ttvbbju_dpd_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;
}

#endif
