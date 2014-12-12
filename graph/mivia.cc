/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/mivia.hh>
#include <graph/graph.hh>
#include <graph/graph_file_error.hh>
#include <fstream>

using namespace parasols;

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

auto parasols::read_mivia(const std::string & filename, const GraphOptions & options) -> Graph
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

    infile.peek();
    if (! infile.eof())
        throw GraphFileError{ filename, "EOF not reached" };

    return result;
}

