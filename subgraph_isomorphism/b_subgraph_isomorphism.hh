/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_B_SUBGRAPH_ISOMORPHISM_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_B_SUBGRAPH_ISOMORPHISM_HH 1

#include <subgraph_isomorphism/subgraph_isomorphism_params.hh>
#include <subgraph_isomorphism/subgraph_isomorphism_result.hh>
#include <graph/graph.hh>

#include <thread>
#include <map>
#include <list>
#include <chrono>
#include <atomic>

namespace parasols
{
    auto b_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto bdds_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto bddsc_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto bnds_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto bndsdds_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto bndsddsc_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;
}

#endif
