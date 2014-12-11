/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_ADJ_HH
#define PARASOLS_GUARD_GRAPH_ADJ_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Read a adj format file into a Graph.
     *
     * \throw GraphFileError
     */
    auto read_adj(const std::string & filename) -> Graph;
}

#endif
