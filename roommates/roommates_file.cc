/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <roommates/roommates_file.hh>
#include <fstream>
#include <functional>

using namespace parasols;

InvalidRoommatesFile::InvalidRoommatesFile(const std::string & filename, const std::string & message) throw () :
    _what("Error reading roommates file '" + filename + "': " + message)
{
}

auto InvalidRoommatesFile::what() const throw () -> const char *
{
    return _what.c_str();
}

auto parasols::read_roommates(const std::string & filename) -> RoommatesProblem
{
    std::ifstream infile{ filename };
    if (! infile)
        throw InvalidRoommatesFile{ filename, "unable to open file" };

    RoommatesProblem result;

    infile >> result.size;

    for (auto c : { std::ref(result.preferences), std::ref(result.rankings) }) {
        c.get().resize(result.size);
        for (auto & cc : c.get())
            cc.resize(result.size);
    }

    unsigned k;
    for (unsigned i = 0 ; i < result.size ; ++i) {
        for (unsigned j = 0 ; j < result.size - 1 ; ++j) {
            infile >> k;
            --k;
            result.preferences[i][j] = k;
            result.rankings[i][k] = j;
        }
        result.preferences[i][result.size - 1] = i;
        result.rankings[i][i] = result.size - 1;
    }

    if (! infile)
        throw InvalidRoommatesFile{ filename, "error reading file" };

    return result;
}

