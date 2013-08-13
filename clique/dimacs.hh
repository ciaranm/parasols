/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CLIQUE_GUARD_CLIQUE_DIMACS_HH
#define CLIQUE_GUARD_CLIQUE_DIMACS_HH 1

#include <clique/graph.hh>
#include <string>

namespace clique
{
    /**
     * Thrown if we come across bad data in a DIMACS format file.
     */
    class InvalidDIMACSFile :
        public std::exception
    {
        private:
            std::string _what;

        public:
            InvalidDIMACSFile(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };

    /**
     * Read a DIMACS format file into a Graph.
     *
     * \throw InvalidDIMACSFile
     */
    auto read_dimacs(const std::string & filename) -> Graph;
}

#endif
