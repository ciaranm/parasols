/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_COMMON_SUBGRAPH_ALGORITHMS_HH
#define PARASOLS_GUARD_MAX_COMMON_SUBGRAPH_ALGORITHMS_HH 1

#include <max_common_subgraph/clique_max_common_subgraph.hh>

#include <utility>
#include <string>

namespace parasols
{
    auto max_common_subgraph_algorithms = {
        std::make_pair( std::string{ "c" },     clique_max_common_subgraph)
    };
}

#endif
