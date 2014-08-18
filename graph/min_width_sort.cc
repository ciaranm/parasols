/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/min_width_sort.hh>

#include <algorithm>

using namespace parasols;

auto parasols::min_width_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void
{
    // pre-calculate degrees
    std::vector<int> degrees;
    std::transform(p.begin(), p.end(), std::back_inserter(degrees),
            [&] (int v) { return graph.degree(v); });

    std::vector<int> result;

    while (! p.empty()) {
        auto min = std::min_element(p.begin(), p.end(),
                [&] (int a, int b) { return degrees[a] < degrees[b] || (degrees[a] == degrees[b] && a > b); });

        result.push_back(*min);

        for (auto & v : p)
            if (graph.adjacent(*min, v))
                --degrees[v];

        p.erase(min);
    }

    if (reverse)
        std::swap(result, p);
    else
        for (auto r = result.rbegin() ; r != result.rend() ; ++r)
            p.push_back(*r);
}

auto parasols::mwsi_sort(const Graph & graph, std::vector<int> & p) -> void
{
    // pre-calculate degrees
    std::vector<int> degrees;
    std::transform(p.begin(), p.end(), std::back_inserter(degrees),
            [&] (int v) { return graph.degree(v); });

    auto unadulterated_degrees = degrees;

    // pre-calculate supports
    std::vector<int> exdegrees((graph.size()));
    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (graph.adjacent(i, j))
                exdegrees[i] += degrees[j];

    std::vector<int> result;

    while (! p.empty()) {
        auto min = std::min_element(p.begin(), p.end(),
                [&] (int a, int b) { return
                (degrees[a] < degrees[b]) ||
                (degrees[a] == degrees[b] && exdegrees[a] < exdegrees[b]) ||
                (degrees[a] == degrees[b] && exdegrees[a] == exdegrees[b] && a < b); });

        result.push_back(*min);

        for (auto & v : p)
            if (graph.adjacent(*min, v))
                --degrees[v];

        p.erase(min);

        for (int i = 0 ; i < graph.size() ; ++i) {
            exdegrees[i] = 0;
            for (auto & v : p)
                if (graph.adjacent(i, v))
                    exdegrees[i] += degrees[v];
        }
    }

    for (auto r = result.rbegin() ; r != result.rend() ; ++r)
        p.push_back(*r);

    // now the sort step
    std::stable_sort(p.begin(), p.begin() + (p.size() / 4),
                [&] (int a, int b) { return (unadulterated_degrees[a] > unadulterated_degrees[b]); });
}

auto parasols::mwssi_sort(const Graph & graph, std::vector<int> & p) -> void
{
    // pre-calculate degrees
    std::vector<int> degrees;
    std::transform(p.begin(), p.end(), std::back_inserter(degrees),
            [&] (int v) { return graph.degree(v); });

    auto unadulterated_degrees = degrees;

    // pre-calculate supports
    std::vector<int> exdegrees((graph.size()));
    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (graph.adjacent(i, j))
                exdegrees[i] += degrees[j];

    std::vector<int> result;

    while (! p.empty()) {
        auto min = std::min_element(p.begin(), p.end(),
                [&] (int a, int b) { return
                (degrees[a] < degrees[b]) ||
                (degrees[a] == degrees[b] && exdegrees[a] < exdegrees[b]) ||
                (degrees[a] == degrees[b] && exdegrees[a] == exdegrees[b] && a < b); });

        result.push_back(*min);

        for (auto & v : p)
            if (graph.adjacent(*min, v))
                --degrees[v];

        p.erase(min);
    }

    for (auto r = result.rbegin() ; r != result.rend() ; ++r)
        p.push_back(*r);

    // now the sort step
    std::stable_sort(p.begin(), p.begin() + (p.size() / 4),
                [&] (int a, int b) { return (unadulterated_degrees[a] > unadulterated_degrees[b]); });
}

