/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_MIVIA_HH
#define PARASOLS_GUARD_GRAPH_MIVIA_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Thrown if we come across bad data in a MIVIA format file.
     */
    class InvalidMIVIAFile :
        public std::exception
    {
        private:
            std::string _what;

        public:
            InvalidMIVIAFile(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };

    /**
     * Read a MIVIA format file into a Graph.
     *
     * \throw InvalidMIVIAFile
     */
    auto read_mivia(const std::string & filename) -> Graph;
}

#endif
