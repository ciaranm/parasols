/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_GRAPH_FILE_ERROR_HH
#define PARASOLS_GUARD_GRAPH_GRAPH_FILE_ERROR_HH 1

#include <exception>
#include <string>

namespace parasols
{
    /**
     * Thrown if we come across bad data in a graph file, or if we can't read a
     * graph file.
     */
    class GraphFileError :
        public std::exception
    {
        private:
            std::string _what;

        public:
            GraphFileError(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };
}

#endif
