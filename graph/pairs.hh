/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_PAIRS_HH
#define PARASOLS_GUARD_GRAPH_PAIRS_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Read a (v,v) or (v v) format file into a Graph.
     *
     * The first number is the number of vertices. The second is the number of
     * edges (this is often wrong, and is ignored). Subsequent lines are pairs
     * v1,v2 or v1 v2 of edges.
     *
     * \throw GraphFileError
     */
    auto read_pairs(const std::string & filename, bool one_indexed) -> Graph;
}

#endif
