/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/metis.hh>
#include <graph/graph.hh>
#include <graph/graph_file_error.hh>
#include <boost/regex.hpp>
#include <fstream>

using namespace parasols;

auto parasols::read_metis(const std::string & filename, const GraphOptions & options) -> Graph
{
    Graph result(0, true);

    std::ifstream infile{ filename };
    if (! infile)
        throw GraphFileError{ filename, "unable to open file" };

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
                    throw GraphFileError{ filename, "unsupported fmt " + match.str(4) + " is not 0 or 1" };
            }

            if (! match.str(6).empty())
                if (match.str(6) != "0")
                    throw GraphFileError{ filename, "unsupported ncon " + match.str(6) + " is not 0" };

            break;
        }
        else
            throw GraphFileError{ filename, "could not parse first line" };
    }

    if (0 == result.size())
        throw GraphFileError{ filename, "no problem line found" };

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
                    throw GraphFileError{ filename, "bad edge destination" };

                if (e == row && ! test(options, GraphOptions::AllowLoops))
                    throw GraphFileError{ filename, "loop detected" };

                result.add_edge(row - 1, e - 1);
                if (weighted_edges)
                    line_s >> e;
            }

            if (! line_s.eof())
                throw GraphFileError{ filename, "bad edges line" };
        }

        if (row == result.size())
            break;
    }

    while (std::getline(infile, line)) {
        boost::smatch match;
        if ((! line.empty()) && (! regex_match(line, match, comment)))
            throw GraphFileError{ filename, "trailing non-empty lines" };
    }

    if (row != result.size())
        throw GraphFileError{ filename, "not enough lines read" };

    if (! infile.eof())
        throw GraphFileError{ filename, "error reading file" };

    return result;
}

