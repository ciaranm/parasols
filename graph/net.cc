/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/net.hh>
#include <graph/graph.hh>
#include <graph/graph_file_error.hh>
#include <boost/regex.hpp>
#include <fstream>

using namespace parasols;

auto parasols::read_net(const std::string & filename) -> Graph
{
    Graph result(0, true);

    std::ifstream infile{ filename };
    if (! infile)
        throw GraphFileError{ filename, "unable to open file" };

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        if (line.at(line.length() - 1) == '\r')
            line.erase(line.length() - 1);

        static const boost::regex
            comment{ R"((%.*)?)" },
            problem{ R"(\*\s*Vertices\s+(\d+))" },
            description{ R"(\d+\s+".*")" },
            arcs{ R"(\*\s*Arcslist)" },
            edge_start{ R"(\*\s*Edgeslist)" };

        boost::smatch match;
        if (regex_match(line, match, comment)) {
        }
        else if (regex_match(line, match, description) || regex_match(line, match, arcs)) {
        }
        else if (regex_match(line, match, problem)) {
            if (0 != result.size())
                throw GraphFileError{ filename, "multiple '*Vertices' lines encountered" };
            result.resize(std::stoi(match.str(1)));
        }
        else if (regex_match(line, match, edge_start)) {
            break;
        }
        else
            throw GraphFileError{ filename, "cannot parse line '" + line + "'" };
    }

    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        if (line.at(line.length() - 1) == '\r')
            line.erase(line.length() - 1);

        std::stringstream line_s{ line };
        int f, t;

        if (! (line_s >> f))
            throw GraphFileError{ filename, "cannot parse edge line '" + line + "'" };
        --f;

        if (f < 0 || f >= result.size())
            throw GraphFileError{ filename, "invalid f value" };

        while (line_s >> t) {
            --t;
            if (t < 0 || t >= result.size() || t == f)
                throw GraphFileError{ filename, "invalid t value " + std::to_string(t) + " (" + std::to_string(f) + ", " + std::to_string(result.size()) + ")" };
            result.add_edge(f, t);
        }

        if (! line_s.eof())
            throw GraphFileError{ filename, "cannot parse edge line '" + line + "'" };
    }

    if (! infile.eof())
        throw GraphFileError{ filename, "error reading file" };

    return result;
}

