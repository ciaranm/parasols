/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/lad.hh>
#include <graph/graph.hh>
#include <fstream>

using namespace parasols;

namespace
{
    auto read_word(std::ifstream & infile) -> int
    {
        int x;
        infile >> x;
        return x;
    }
}

auto parasols::read_lad(const std::string & filename, const GraphOptions & options) -> Graph
{
    Graph result(0, false);

    std::ifstream infile{ filename };
    if (! infile)
        throw GraphFileError{ filename, "unable to open file" };

    result.resize(read_word(infile));
    if (! infile)
        throw GraphFileError{ filename, "error reading size" };

    for (int r = 0 ; r < result.size() ; ++r) {
        int c_end = read_word(infile);
        if (! infile)
            throw GraphFileError{ filename, "error reading edges count" };

        for (int c = 0 ; c < c_end ; ++c) {
            int e = read_word(infile);

            if (e < 0 || e >= result.size())
                throw GraphFileError{ filename, "edge index out of bounds" };
            else if (r == e && ! test(options, GraphOptions::AllowLoops))
                throw GraphFileError{ filename, "loop on vertex " + std::to_string(r) };

            result.add_edge(r, e);
        }
    }

    std::string rest;
    if (infile >> rest)
        throw GraphFileError{ filename, "EOF not reached, next text is \"" + rest + "\"" };
    if (! infile.eof())
        throw GraphFileError{ filename, "EOF not reached" };

    return result;
}

