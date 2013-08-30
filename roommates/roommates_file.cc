/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <roommates/roommates_file.hh>
#include <fstream>

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

    return result;
}

