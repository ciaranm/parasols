/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_VERTEX_COLOURING_NAIVE_HH
#define PARASOLS_GUARD_VERTEX_COLOURING_NAIVE_HH 1

#include <graph/graph.hh>
#include <vertex_colouring/vertex_colouring_params.hh>
#include <vertex_colouring/vertex_colouring_result.hh>

namespace parasols
{
    /**
     * Very stupid colouring algorithm.
     */
    auto naive_vertex_colouring(const Graph & graph, const VertexColouringParams &) -> VertexColouringResult;
}

#endif
