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

    unsigned size, j;
    infile >> size;

    for (auto c : { std::ref(result.preferences), std::ref(result.ranks) }) {
        c.get().resize(size);
        for (auto & cc : c.get())
            cc.resize(size);
    }

    for (unsigned i = 0 ; i < size ; ++i) {
        for (unsigned k = 0 ; k < size - 1 ; ++k) {
            infile >> j;
            --j;
            result.preferences.at(i).at(j) = k;
            result.ranks.at(i).at(k) = j;
        }
        result.ranks.at(i).at(size - 1) = i;
        result.preferences.at(i).at(i) = size - 1;
    }

    return result;
}

