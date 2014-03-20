/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_kclub/naive_max_kclub.hh>
#include <graph/is_club.hh>
#include <algorithm>

using namespace parasols;

namespace
{
    auto expand(
            const Graph & graph,
            std::vector<int> & c,                    // current candidate clique
            std::vector<int> & p,                    // potential additions
            MaxKClubResult & result,
            const MaxKClubParams & params) -> void
    {
        ++result.nodes;

        while (! p.empty()) {
            if (c.size() + p.size() <= result.size || result.size >= params.stop_after_finding || params.abort->load())
                return;

            auto v = p.back();
            p.pop_back();

            // consider v in c
            c.push_back(v);

            // club?
            if (c.size() > result.size && is_club(graph, params.k, c)) {
                result.members = std::set<int>{ c.begin(), c.end() };
                result.size = c.size();
            }

            if (! p.empty()) {
                auto new_p = p;
                expand(graph, c, new_p, result, params);
            }

            // consider v notin c
            c.pop_back();
        }
    }
}

auto parasols::naive_max_kclub(const Graph & graph, const MaxKClubParams & params) -> MaxKClubResult
{
    MaxKClubResult result;
    result.size = params.initial_bound;

    std::vector<int> c; // current candidate clique
    c.reserve(graph.size());

    std::vector<int> p(graph.size()); // potential additions
    std::iota(p.begin(), p.end(), 0);

    expand(graph, c, p, result, params);

    return result;
}

