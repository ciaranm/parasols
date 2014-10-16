/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_ADJ_HH
#define PARASOLS_GUARD_GRAPH_ADJ_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Thrown if we come across bad data in an adj (from GAP) format file.
     */
    class InvalidADJFile :
        public std::exception
    {
        private:
            std::string _what;

        public:
            InvalidADJFile(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };

    /**
     * Read a adj format file into a Graph.
     *
     * \throw InvalidADJFile
     */
    auto read_adj(const std::string & filename) -> Graph;
}

#endif
