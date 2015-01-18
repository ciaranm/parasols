/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_CB_SUBGRAPH_ISOMORPHISM_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_CB_SUBGRAPH_ISOMORPHISM_HH 1

#include <subgraph_isomorphism/subgraph_isomorphism_params.hh>
#include <subgraph_isomorphism/subgraph_isomorphism_result.hh>
#include <graph/graph.hh>

namespace parasols
{
    auto cbjp_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjpi_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjpi_rand_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjpi_randdeg_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto csbjpi_randdeg_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cpi_randdeg_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;
}

#endif
