/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_MIVIA_HH
#define PARASOLS_GUARD_GRAPH_MIVIA_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Read a MIVIA format file into a Graph.
     *
     * \throw GraphFileError
     */
    auto read_mivia(const std::string & filename, const GraphOptions &) -> Graph;
}

#endif
