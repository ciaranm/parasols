/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_LV_HH
#define PARASOLS_GUARD_GRAPH_LV_HH 1

#include <graph/graph.hh>
#include <string>

namespace parasols
{
    /**
     * Thrown if we come across bad data in a LV format file.
     */
    class InvalidLVFile :
        public std::exception
    {
        private:
            std::string _what;

        public:
            InvalidLVFile(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };

    /**
     * Read a LV format file into a Graph.
     *
     * \throw InvalidLVFile
     */
    auto read_lv(const std::string & filename) -> Graph;
}

#endif
