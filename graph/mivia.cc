/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/mivia.hh>
#include <graph/graph.hh>
#include <boost/regex.hpp>
#include <fstream>

using namespace parasols;

InvalidMIVIAFile::InvalidMIVIAFile(const std::string & filename, const std::string & message) throw () :
    _what("Error reading MIVIA file '" + filename + "': " + message)
{
}

auto InvalidMIVIAFile::what() const throw () -> const char *
{
    return _what.c_str();
}

namespace
{
    auto read_word(std::ifstream & infile) -> int
    {
        unsigned char a, b;
        a = static_cast<unsigned char>(infile.get());
        b = static_cast<unsigned char>(infile.get());
        return int(a) | (int(b) << 8);
    }
}

auto parasols::read_mivia(const std::string & filename) -> Graph
{
    Graph result(0, false);

    std::ifstream infile{ filename };
    if (! infile)
        throw InvalidMIVIAFile{ filename, "unable to open file" };

    result.resize(read_word(infile));
    if (! infile)
        throw InvalidMIVIAFile{ filename, "error reading size" };

    for (int r = 0 ; r < result.size() ; ++r) {
        int c_end = read_word(infile);
        if (! infile)
            throw InvalidMIVIAFile{ filename, "error reading edges count" };

        for (int c = 0 ; c < c_end ; ++c) {
            int e = read_word(infile);

            if (e < 0 || e >= result.size())
                throw InvalidMIVIAFile{ filename, "edge index out of bounds" };
            else if (r == e)
                throw InvalidMIVIAFile{ filename, "contains a loop" };

            result.add_edge(r, e);
        }
    }

    infile.peek();
    if (! infile.eof())
        throw InvalidMIVIAFile{ filename, "EOF not reached" };

    return result;
}

