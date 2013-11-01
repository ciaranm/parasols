/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_IS_CLIQUE_HH
#define PARASOLS_GUARD_GRAPH_IS_CLIQUE_HH 1

#include <graph/graph.hh>
#include <set>

namespace parasols
{
    auto is_clique(const Graph & graph, const std::set<int> & members) -> bool;
}

#endif
