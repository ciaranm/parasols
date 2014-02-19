/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_KCLUB_KNEIGHBOURS_HH
#define PARASOLS_GUARD_MAX_KCLUB_KNEIGHBOURS_HH 1

#include <max_kclub/max_kclub_params.hh>
#include <max_kclub/max_kclub_result.hh>

#include <vector>
#include <set>

namespace parasols
{
    struct VertexDistance
    {
        int distance;
        int next;
    };

    struct VertexInformation
    {
        std::vector<int> firsts;
        std::vector<VertexDistance> distances;
    };

    struct KNeighbours
    {
        std::vector<VertexInformation> vertices;

        KNeighbours(const Graph & graph, const MaxKClubParams & params, const std::vector<int> * maybe_restrict = nullptr);
    };
}

#endif
