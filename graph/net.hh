/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_NET_HH
#define PARASOLS_GUARD_GRAPH_NET_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Thrown if we come across bad data in a file.
     */
    class InvalidNetFile :
        public std::exception
    {
        private:
            std::string _what;

        public:
            InvalidNetFile(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };

    /**
     * Read a net format file into a Graph.
     *
     * \throw InvalidNetFile
     */
    auto read_net(const std::string & filename) -> Graph;
}

#endif
