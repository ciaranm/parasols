/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <clique/mcsa1_max_clique.hh>
#include <clique/degree_sort.hh>
#include <clique/colourise.hh>
#include <algorithm>

using namespace clique;

namespace
{
    auto expand(
            const Graph & graph,
            Buckets & buckets,                       // pre-allocated
            std::vector<int> & c,                    // current candidate clique
            std::vector<int> & o,                    // potential additions, in order
            MaxCliqueResult & result,
            const MaxCliqueParams & params) -> void
    {
        ++result.nodes;

        // get our coloured vertices
        std::vector<int> p;
        p.reserve(o.size());
        auto colours = colourise(graph, buckets, p, o);

        // for each v in p... (v comes later)
        for (int n = p.size() - 1 ; n >= 0 ; --n) {

            // bound, timeout or early exit?
            if (c.size() + colours[n] <= result.size || result.size >= params.stop_after_finding || params.abort.load())
                return;

            auto v = p[n];

            // consider taking v
            c.push_back(v);

            // filter o to contain vertices adjacent to v
            std::vector<int> new_o;
            new_o.reserve(o.size());
            std::copy_if(o.begin(), o.end(), std::back_inserter(new_o), [&] (int w) { return graph.adjacent(v, w); });

            if (new_o.empty()) {
                // potential new best
                if (c.size() > result.size) {
                    result.size = c.size();
                    result.members = std::set<int>{ c.begin(), c.end() };
                    if (params.print_candidates)
                        print_candidate(params, result.size);
                }
            }
            else
                expand(graph, buckets, c, new_o, result, params);

            // now consider not taking v
            c.pop_back();
            p.pop_back();
            o.erase(std::remove(o.begin(), o.end(), v));
        }
    }
}

auto clique::mcsa1_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    MaxCliqueResult result;
    result.size = params.initial_bound;

    std::vector<int> c; // current candidate clique
    c.reserve(graph.size());

    std::vector<int> o(graph.size()); // potential additions, ordered
    std::iota(o.begin(), o.end(), 0);
    degree_sort(graph, o);

    auto buckets = make_buckets(graph.size());

    // go!
    expand(graph, buckets, c, o, result, params);

    return result;
}

