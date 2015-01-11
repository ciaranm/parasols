/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_CB_SUBGRAPH_ISOMORPHISM_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_CB_SUBGRAPH_ISOMORPHISM_HH 1

#include <subgraph_isomorphism/subgraph_isomorphism_params.hh>
#include <subgraph_isomorphism/subgraph_isomorphism_result.hh>
#include <graph/graph.hh>

namespace parasols
{
    auto cb_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbj_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbd_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjd_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjdnoalldiff_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjdnom3_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjdnom23_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjdnod3_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjdnod23_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbfast_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjfast_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbdfast_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjdfast_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjdprobe1_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjdprobe2_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;

    auto cbjdprobe3_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;
}

#endif
