/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_DIMACS_HH
#define PARASOLS_GUARD_GRAPH_DIMACS_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Read a DIMACS format file into a Graph.
     *
     * \throw GraphFileError
     */
    auto read_dimacs(const std::string & filename, const GraphOptions & options) -> Graph;
}

#endif
