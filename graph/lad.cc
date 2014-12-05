/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/lad.hh>
#include <graph/graph.hh>
#include <fstream>

using namespace parasols;

InvalidLADFile::InvalidLADFile(const std::string & filename, const std::string & message) throw () :
    _what("Error reading LAD file '" + filename + "': " + message)
{
}

auto InvalidLADFile::what() const throw () -> const char *
{
    return _what.c_str();
}

namespace
{
    auto read_word(std::ifstream & infile) -> int
    {
        int x;
        infile >> x;
        return x;
    }
}

auto parasols::read_lad(const std::string & filename) -> Graph
{
    Graph result(0, false);

    std::ifstream infile{ filename };
    if (! infile)
        throw InvalidLADFile{ filename, "unable to open file" };

    result.resize(read_word(infile));
    if (! infile)
        throw InvalidLADFile{ filename, "error reading size" };

    for (int r = 0 ; r < result.size() ; ++r) {
        int c_end = read_word(infile);
        if (! infile)
            throw InvalidLADFile{ filename, "error reading edges count" };

        for (int c = 0 ; c < c_end ; ++c) {
            int e = read_word(infile);

            if (e < 0 || e >= result.size())
                throw InvalidLADFile{ filename, "edge index out of bounds" };
            else if (r == e)
                throw InvalidLADFile{ filename, "contains a loop" };

            result.add_edge(r, e);
        }
    }

    std::string rest;
    if (infile >> rest)
        throw InvalidLADFile{ filename, "EOF not reached, next text is \"" + rest + "\"" };
    if (! infile.eof())
        throw InvalidLADFile{ filename, "EOF not reached" };

    return result;
}

