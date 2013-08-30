/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_ROOMMATES_ROOMMATES_FILE_HH
#define PARASOLS_GUARD_ROOMMATES_ROOMMATES_FILE_HH 1

#include <roommates/roommates_problem.hh>
#include <string>
#include <exception>

namespace parasols
{
    auto read_roommates(const std::string &) -> RoommatesProblem;

    /**
     * Thrown if we come across bad data in a roommates file.
     */
    class InvalidRoommatesFile :
        public std::exception
    {
        private:
            std::string _what;

        public:
            InvalidRoommatesFile(const std::string & filename, const std::string & message) throw ();

            auto what() const throw () -> const char *;
    };
}

#endif
