/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CLIQUE_GUARD_CLIQUE_NAIVE_MAX_CLIQUE_HH
#define CLIQUE_GUARD_CLIQUE_NAIVE_MAX_CLIQUE_HH 1

#include <clique/graph.hh>
#include <clique/max_clique_params.hh>
#include <vector>

namespace clique
{
    /**
     * Very stupid max clique algorithm.
     */
    auto naive_max_clique(const Graph & graph, const MaxCliqueParams &) -> MaxCliqueResult;
}

#endif
