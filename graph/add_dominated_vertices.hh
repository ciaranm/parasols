/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_ADD_DOMINATED_VERTICES_HH
#define PARASOLS_GUARD_GRAPH_ADD_DOMINATED_VERTICES_HH 1

#include <graph/graph.hh>

namespace parasols
{
    /**
     * Add in n additional vertices whose neighbourhood is a subset of the
     * neighbourhood of some other vertex, with edge inclusion probability p.
     * New vertices are made adjacent to their dominating vertex with
     * probability q.
     */
    Graph add_dominated_vertices(const Graph & graph, int n, double p, double q, int seed);
}

#endif
