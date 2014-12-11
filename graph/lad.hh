/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_LAD_HH
#define PARASOLS_GUARD_GRAPH_LAD_HH 1

#include <graph/graph.hh>
#include <graph/graph_file_error.hh>
#include <string>

namespace parasols
{
    /**
     * Read a LAD format file into a Graph.
     *
     * \throw GraphFileError
     */
    auto read_lad(const std::string & filename) -> Graph;
}

#endif
