/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_IS_CLUB_HH
#define PARASOLS_GUARD_GRAPH_IS_CLUB_HH 1

#include <graph/graph.hh>
#include <set>

namespace parasols
{
    auto is_club(const Graph & graph, int k, const std::set<int> & members) -> bool;
}

#endif
