/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_KCLUB_DISTANCE_MAX_KCLUB_HH
#define PARASOLS_GUARD_MAX_KCLUB_DISTANCE_MAX_KCLUB_HH 1

#include <graph/graph.hh>
#include <max_kclub/max_kclub_params.hh>
#include <max_kclub/max_kclub_result.hh>

namespace parasols
{
    /**
     * Moderately stupid max k-club algorithm.
     */
    auto distance_max_kclub(const Graph & graph, const MaxKClubParams &) -> MaxKClubResult;
}

#endif
