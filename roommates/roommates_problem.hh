/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_ROOMMATES_ROOMMATES_PROBLEM_HH
#define PARASOLS_GUARD_ROOMMATES_ROOMMATES_PROBLEM_HH 1

#include <vector>

namespace parasols
{
    struct RoommatesProblem
    {
        unsigned size;
        std::vector<std::vector<unsigned> > preferences;
        std::vector<std::vector<unsigned> > rankings;
    };
}

#endif
