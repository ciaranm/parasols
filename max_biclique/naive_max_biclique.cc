/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_biclique/naive_max_biclique.hh>

using namespace parasols;

auto parasols::naive_max_biclique(const Graph &, const MaxBicliqueParams & params) -> MaxBicliqueResult
{
    MaxBicliqueResult result;
    result.size = params.initial_bound;

    return result;
}

