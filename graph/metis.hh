/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_METIS_HH
#define PARASOLS_GUARD_GRAPH_METIS_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Read a METIS format file into a Graph.
     *
     * \throw GraphFileError
     */
    auto read_metis(const std::string & filename) -> Graph;
}

#endif
