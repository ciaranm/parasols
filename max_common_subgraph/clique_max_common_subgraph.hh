/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_COMMON_SUBGRAPH_CLIQUE_MAX_COMMON_SUBGRAPH_HH
#define PARASOLS_GUARD_MAX_COMMON_SUBGRAPH_CLIQUE_MAX_COMMON_SUBGRAPH_HH 1

#include <max_common_subgraph/max_common_subgraph_params.hh>
#include <max_common_subgraph/max_common_subgraph_result.hh>

namespace parasols
{
    /**
     * Clique-based maximum common subgraph algorithm
     */
    auto clique_max_common_subgraph(const std::pair<Graph, Graph> & graphs, const MaxCommonSubgraphParams & params) -> MaxCommonSubgraphResult;
}

#endif
