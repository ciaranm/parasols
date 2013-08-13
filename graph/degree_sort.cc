/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/degree_sort.hh>

#include <algorithm>
#include <iterator>

using namespace parasols;

auto parasols::degree_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void
{
    // pre-calculate degrees
    std::vector<int> degrees;
    std::transform(p.begin(), p.end(), std::back_inserter(degrees),
            [&] (int v) { return graph.degree(v); });

    // sort on degree
    std::sort(p.begin(), p.end(),
            [&] (int a, int b) { return (! reverse) ^ (degrees[a] < degrees[b] || (degrees[a] == degrees[b] && a > b)); });
}

