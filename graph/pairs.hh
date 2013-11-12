/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_PAIRS_HH
#define PARASOLS_GUARD_GRAPH_PAIRS_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Thrown if we come across bad data in a file.
     */
    class InvalidPairsFile :
        public std::exception
    {
        private:
            std::string _what;

        public:
            InvalidPairsFile(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };

    /**
     * Read a (v,v) format file into a Graph.
     *
     * The first line is the number of vertices. The second is the number of
     * edges (this is often wrong, and is ignored). Subsequent lines are pairs
     * v1,v2 of edges.
     *
     * \throw InvalidPairsFile
     */
    auto read_pairs(const std::string & filename) -> Graph;
}

#endif
