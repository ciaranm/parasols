/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <clique/naive_max_clique.hh>
#include <boost/range/adaptors.hpp>
#include <algorithm>

using namespace clique;

namespace
{
    auto expand(
            const Graph & graph,
            std::vector<std::vector<int> > & palloc, // pre-allocated space for p
            std::vector<int> & c,                    // current candidate clique
            std::vector<int> & p,                    // potential additions
            MaxCliqueResult & result,
            const MaxCliqueParams & params
            ) -> void
    {
        ++result.nodes;

        // for each v in p...
        for (auto v : p | boost::adaptors::reversed) {

            // bound, timeout or early exit?
            if (c.size() + p.size() <= result.size || result.size >= params.stop_after_finding || params.abort.load())
                return;

            // consider taking v
            c.push_back(v);

            // filter p to contain vertices adjacent to v
            std::vector<int> & new_p = palloc[c.size()];
            new_p.clear();
            std::copy_if(p.begin(), p.end(), std::back_inserter(new_p), [&] (int w) { return graph.adjacent(v, w); });

            if (new_p.empty()) {
                // potential new best
                if (c.size() > result.size) {
                    result.size = c.size();
                    result.members = std::set<int>{ c.begin(), c.end() };
                    if (params.print_candidates)
                        print_candidate(params, result.size);
                }
            }
            else
                expand(graph, palloc, c, new_p, result, params);

            // now consider not taking v
            c.pop_back();
            p.pop_back();
        }
    }
}

auto clique::naive_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    MaxCliqueResult result;
    result.size = params.initial_bound;

    std::vector<int> c; // current candidate clique
    c.reserve(graph.size());

    std::vector<int> p(graph.size()); // potential additions

    // pre-allocate space for p in recursive calls
    std::vector<std::vector<int> > palloc(graph.size());
    for (auto & pa : palloc)
        pa.reserve(graph.size());

    // populate p with every vertex initially
    std::iota(p.begin(), p.end(), 0);

    // go!
    expand(graph, palloc, c, p, result, params);

    return result;
}

