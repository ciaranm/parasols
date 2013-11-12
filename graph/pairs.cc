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

auto parasols::read_pairs(const std::string & filename) -> Graph
{
    Graph result(0, false);

    std::ifstream infile{ filename };
    if (! infile)
        throw InvalidPairsFile{ filename, "unable to open file" };

    std::string line;

    if (! std::getline(infile, line))
        throw InvalidPairsFile{ filename, "cannot parse number of vertices" };
    result.resize(std::stoi(line));

    std::getline(infile, line);

    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        static const boost::regex edge{ R"((\d+),(\d+)\s*)" };
        boost::smatch match;

        if (regex_match(line, match, edge)) {
            /* An edge. DIMACS files are 1-indexed. We assume we've already had
             * a problem line (if not our size will be 0, so we'll throw). */
            int a{ std::stoi(match.str(1)) }, b{ std::stoi(match.str(2)) };
            if (a >= result.size() || b >= result.size())
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

