/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/pairs.hh>
#include <graph/graph.hh>
#include <graph/graph_file_error.hh>
#include <boost/regex.hpp>
#include <fstream>

using namespace parasols;

auto parasols::read_pairs(const std::string & filename, bool one_indexed, const GraphOptions & options) -> Graph
{
    std::ifstream infile{ filename };
    if (! infile)
        throw GraphFileError{ filename, "unable to open file" };

    std::string line;

    static const boost::regex double_header{ R"((\d+)\s+(\d+)\s*(\d+)?)" };
    unsigned size;

    if (! std::getline(infile, line))
        throw GraphFileError{ filename, "cannot parse number of vertices" };

    {
        boost::smatch match;
        if (regex_match(line, match, double_header))
            size = std::stoi(match.str(1));
        else {
            size = std::stoi(line);
            std::getline(infile, line);
        }
    }

    Graph result(size, one_indexed);

    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        static const boost::regex edge{ R"((\d+)(,|\s+)(\d+)\s*)" };
        boost::smatch match;

        if (regex_match(line, match, edge)) {
            int a{ std::stoi(match.str(1)) }, b{ std::stoi(match.str(3)) };

            if (one_indexed) {
                --a;
                --b;
            }

            if (a >= result.size() || b >= result.size() || a < 0 || b < 0)
                throw GraphFileError{ filename, "line '" + line + "' edge index out of bounds" };
            else if (a == b && ! test(options, GraphOptions::AllowLoops))
                throw GraphFileError{ filename, "line '" + line + "' contains a loop on vertex " + std::to_string(a) };
            result.add_edge(a, b);
        }
        else
            throw GraphFileError{ filename, "cannot parse line '" + line + "'" };
    }

    if (! infile.eof())
        throw GraphFileError{ filename, "error reading file" };

    return result;
}

