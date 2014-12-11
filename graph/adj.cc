/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/adj.hh>
#include <graph/graph.hh>
#include <graph/graph_file_error.hh>
#include <boost/regex.hpp>
#include <fstream>

using namespace parasols;

auto parasols::read_adj(const std::string & filename) -> Graph
{
    Graph result(0, true);

    std::ifstream infile{ filename };
    if (! infile)
        throw GraphFileError{ filename, "unable to open file" };

    std::string word;
    int depth = 0;
    std::vector<int> row_values;
    int row = 0;

    while (infile >> word) {
        if (word == "[") {
            ++depth;
        }
        else if (word == "]" || word == "],") {
            if (--depth < 0)
                throw GraphFileError{ filename, "too many close brackets" };

            if (0 == row)
                result.resize(row_values.size());

            if (1 == depth) {
                if (unsigned(row_values.size()) != unsigned(result.size()))
                    throw GraphFileError{ filename, "bad row length" };

                for (int i = 0 ; i < result.size() ; ++i)
                    if (row_values.at(i)) {
                        if (i == row)
                            throw GraphFileError{ filename, "loop detected" };
                        result.add_edge(row, i);
                    }

                row_values.clear();
                ++row;
            }
        }
        else if (! word.empty()) {
            if (word.at(word.length() - 1) == ',')
                word.erase(word.length() - 1);

            if (word == "0")
                row_values.push_back(0);
            else if (word == "1")
                row_values.push_back(1);
            else if (word.empty()) {
            }
            else
                throw GraphFileError{ filename, "unexpected token '" + word + "'" };
        }
    }

    if (0 != depth || row != result.size() || ! row_values.empty())
        throw GraphFileError{ filename, "couldn't finish reading file" };

    if (! infile.eof())
        throw GraphFileError{ filename, "error reading file" };

    return result;
}

