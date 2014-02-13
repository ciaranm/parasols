/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/pairs.hh>
#include <graph/graph.hh>
#include <boost/regex.hpp>
#include <fstream>

using namespace parasols;

InvalidPairsFile::InvalidPairsFile(const std::string & filename, const std::string & message) throw () :
    _what("Error reading file '" + filename + "': " + message)
{
}

auto InvalidPairsFile::what() const throw () -> const char *
{
    return _what.c_str();
}

auto parasols::read_pairs(const std::string & filename, bool one_indexed) -> Graph
{
    std::ifstream infile{ filename };
    if (! infile)
        throw InvalidPairsFile{ filename, "unable to open file" };

    std::string line;

    static const boost::regex double_header{ R"((\d+)\s+(\d+)\s*)" };
    unsigned size;

    if (! std::getline(infile, line))
        throw InvalidPairsFile{ filename, "cannot parse number of vertices" };

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
                throw InvalidPairsFile{ filename, "line '" + line + "' edge index out of bounds" };
            else if (a == b)
                throw InvalidPairsFile{ filename, "line '" + line + "' contains a loop" };
            result.add_edge(a, b);
        }
        else
            throw InvalidPairsFile{ filename, "cannot parse line '" + line + "'" };
    }

    if (! infile.eof())
        throw InvalidPairsFile{ filename, "error reading file" };

    return result;
}

