/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_ROOMMATES_IRVING_ROOMMATES_HH
#define PARASOLS_GUARD_ROOMMATES_IRVING_ROOMMATES_HH 1

#include <roommates/roommates_problem.hh>
#include <roommates/roommates_result.hh>
#include <roommates/roommates_params.hh>

namespace parasols
{
    auto irving_roommates(const RoommatesProblem &, const RoommatesParams &) -> RoommatesResult;
}

#endif
