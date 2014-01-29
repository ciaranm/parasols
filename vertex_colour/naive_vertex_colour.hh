/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_VERTEX_COLOUR_NAIVE_VERTEX_COLOUR_HH
#define PARASOLS_GUARD_VERTEX_COLOUR_NAIVE_VERTEX_COLOUR_HH 1

#include <vertex_colour/vertex_colouring_result.hh>
#include <vertex_colour/vertex_colouring_params.hh>
#include <graph/graph.hh>

namespace parasols
{
    auto naive_vertex_colour(const Graph & graph, const VertexColouringParams & params) -> VertexColouringResult;
}

#endif
