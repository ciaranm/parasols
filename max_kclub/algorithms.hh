/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_KCLUB_ALGORITHMS_HH
#define PARASOLS_GUARD_MAX_KCLUB_ALGORITHMS_HH 1

#include <max_kclub/naive_max_kclub.hh>
#include <max_kclub/distance_max_kclub.hh>

#include <utility>
#include <string>

namespace parasols
{
    auto max_kclub_algorithms = {
        std::make_pair( std::string{ "naive" },     naive_max_kclub),
        std::make_pair( std::string{ "distance" },  distance_max_kclub)
    };
}

#endif
