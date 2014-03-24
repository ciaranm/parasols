/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_METIS_HH
#define PARASOLS_GUARD_GRAPH_METIS_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Thrown if we come across bad data in a METIS format file.
     */
    class InvalidMETISFile :
        public std::exception
    {
        private:
            std::string _what;

        public:
            InvalidMETISFile(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };

    /**
     * Read a METIS format file into a Graph.
     *
     * \throw InvalidMETISFile
     */
    auto read_metis(const std::string & filename) -> Graph;
}

#endif
