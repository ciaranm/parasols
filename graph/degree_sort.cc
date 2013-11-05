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

auto parasols::exdegree_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void
{
    // pre-calculate degrees and exdegrees
    std::vector<int> degrees;
    std::transform(p.begin(), p.end(), std::back_inserter(degrees),
            [&] (int v) { return graph.degree(v); });

    std::vector<int> exdegrees((graph.size()));
    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (graph.adjacent(i, j))
                exdegrees[i] += degrees[j];

    // sort on degree
    std::sort(p.begin(), p.end(),
            [&] (int a, int b) { return (! reverse) ^ (
                (degrees[a] < degrees[b]) ||
                (degrees[a] == degrees[b] && exdegrees[a] < exdegrees[b]) ||
                (degrees[a] == degrees[b] && exdegrees[a] == exdegrees[b] && a > b)
                ); });
}

auto parasols::dynexdegree_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void
{
    // pre-calculate degrees and exdegrees
    std::vector<int> degrees;
    std::transform(p.begin(), p.end(), std::back_inserter(degrees),
            [&] (int v) { return graph.degree(v); });

    std::vector<int> exdegrees((graph.size()));
    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (graph.adjacent(i, j))
                exdegrees[i] += degrees[j];

    auto unsorted = p.begin(), unsorted_end = p.end();

    while (unsorted != unsorted_end) {
        // sort on degree
        std::sort(unsorted, unsorted_end,
                [&] (int a, int b) { return (! reverse) ^ (
                    (degrees[a] < degrees[b]) ||
                    (degrees[a] == degrees[b] && exdegrees[a] < exdegrees[b]) ||
                    (degrees[a] == degrees[b] && exdegrees[a] == exdegrees[b] && a > b)
                    ); });

        for (int i = 0 ; i < graph.size() ; ++i)
            if (graph.adjacent(i, *prev(unsorted_end)))
                --degrees[i];

        --unsorted_end;
    }
}

