/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/metis.hh>
#include <graph/graph.hh>
#include <boost/regex.hpp>
#include <fstream>

using namespace parasols;

InvalidMETISFile::InvalidMETISFile(const std::string & filename, const std::string & message) throw () :
    _what("Error reading METIS file '" + filename + "': " + message)
{
}

auto InvalidMETISFile::what() const throw () -> const char *
{
    return _what.c_str();
}

auto parasols::read_metis(const std::string & filename) -> Graph
{
    Graph result(0, true);

    std::ifstream infile{ filename };
    if (! infile)
        throw InvalidMETISFile{ filename, "unable to open file" };

    /* Lines are comments, a problem description (contains the number of
     * vertices), or an edge. */
    static const boost::regex
        comment{ R"(%.*)" },
        problem{ R"((\d+)\s+(\d+)(\s+(\d+)(\s+(\d+))?)?)" };

    bool weighted_edges = false;
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        boost::smatch match;
        if (regex_match(line, match, comment)) {
            /* comment */
        }
        else if (regex_match(line, match, problem)) {
            result.resize(std::stoi(match.str(1)));
            if (! match.str(4).empty()) {
                if (match.str(4) == "1")
                    weighted_edges = true;
                else if (match.str(4) != "0")
                    throw InvalidMETISFile{ filename, "unsupported fmt " + match.str(4) + " is not 0 or 1" };
            }

            if (! match.str(6).empty())
                if (match.str(6) != "0")
                    throw InvalidMETISFile{ filename, "unsupported ncon " + match.str(6) + " is not 0" };

            break;
        }
        else
            throw InvalidMETISFile{ filename, "could not parse first line" };
    }

    if (0 == result.size())
        throw InvalidMETISFile{ filename, "no problem line found" };

    int row = 0;
    while (std::getline(infile, line)) {
        boost::smatch match;
        if (regex_match(line, match, comment)) {
            /* comment */
        }
        else {
            ++row;
            std::stringstream line_s{ line };
            int e;
            while (line_s >> e) {
                if (e > result.size() || e < 1)
                    throw InvalidMETISFile{ filename, "bad edge destination" };

                result.add_edge(row - 1, e - 1);
                if (weighted_edges)
                    line_s >> e;
            }

            if (! line_s.eof())
                throw InvalidMETISFile{ filename, "bad edges line" };
        }

        if (row == result.size())
            break;
    }

    while (std::getline(infile, line)) {
        boost::smatch match;
        if ((! line.empty()) && (! regex_match(line, match, comment)))
            throw InvalidMETISFile{ filename, "trailing non-empty lines" };
    }

    if (row != result.size())
        throw InvalidMETISFile{ filename, "not enough lines read" };

    if (! infile.eof())
        throw InvalidMETISFile{ filename, "error reading file" };

    return result;
}

