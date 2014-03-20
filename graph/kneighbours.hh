/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_KNEIGHBOURS_HH
#define PARASOLS_GUARD_GRAPH_KNEIGHBOURS_HH 1

#include <graph/graph.hh>
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

        KNeighbours(const Graph & graph, const int k, const std::vector<int> * maybe_restrict = nullptr);
    };
}

#endif
