/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/dkrtj_sort.hh>
#include <max_clique/colourise.hh>

#include <algorithm>
#include <iterator>

#include <iostream>

auto parasols::dkrtj_sort(const Graph & graph, std::vector<int> & p) -> void
{
    // pre-calculate degrees and ex-degrees
    std::vector<std::pair<unsigned, unsigned> > degrees;
    unsigned max_degree = 0;
    std::transform(p.begin(), p.end(), std::back_inserter(degrees),
            [&] (int v) { return std::make_pair(graph.degree(v), 0); });

    for (auto & v : p) {
        if (degrees[v].first > max_degree)
            max_degree = degrees[v].first;

        for (auto & w : p)
            if (graph.adjacent(v, w))
                degrees[v].second += degrees[w].first;
    }

    auto orig_degrees = degrees;

    for (auto p_end_unsorted = p.end() ; p_end_unsorted != p.begin() ; ) {
        // find vertex of minumum degree, with ex-degree tiebreaking. also find
        // the vertex of max degree, so we can see if all our degrees are the
        // same.
        auto p_min_max = std::minmax_element(p.begin(), p_end_unsorted,
                [&] (int a, int b) { return (
                    (degrees[a].first < degrees[b].first) ||
                    (degrees[a].first == degrees[b].first && degrees[a].second < degrees[b].second) ||
                    (degrees[a] == degrees[b] && a > b)); });

        // does everything remaining have the same degree?
        // if (degrees[*p_min_max.first].first == degrees[*p_min_max.second].first) {
            // DKRTJ uses some kind of colour ordering here
        // }

        // update degrees, but not ex-degrees, because it's what DKRTJ does.
        for (auto & w : p)
            if (graph.adjacent(*p_min_max.first, w))
                --degrees[w].first;

        // move to end
        std::swap(*p_min_max.first, *(p_end_unsorted - 1));
        --p_end_unsorted;
    }
}

