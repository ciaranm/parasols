/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_KCLUB_PRINT_INCUMBENT_HH
#define PARASOLS_GUARD_MAX_KCLUB_PRINT_INCUMBENT_HH 1

#include <graph/graph.hh>
#include <max_kclub/max_kclub_params.hh>
#include <vector>
#include <set>

namespace parasols
{
    /**
     * Do some output, if params.print_incumbents is true.
     *
     * This version supports positions.
     */
    auto print_incumbent(const MaxKClubParams & params, unsigned size, unsigned old_best,
            bool feasible, const std::vector<int> & positions) -> void;
}

#endif
