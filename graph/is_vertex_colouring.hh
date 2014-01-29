/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_IS_VERTEX_COLOURING_HH
#define PARASOLS_GUARD_GRAPH_IS_VERTEX_COLOURING_HH 1

#include <graph/graph.hh>
#include <vector>

namespace parasols
{
    auto is_vertex_colouring(const Graph & graph, const std::vector<int> & colouring) -> bool;
}

#endif
