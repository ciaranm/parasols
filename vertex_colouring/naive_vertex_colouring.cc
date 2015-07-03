/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <vertex_colouring/naive_vertex_colouring.hh>

using namespace parasols;

auto parasols::naive_vertex_colouring(const Graph & graph, const VertexColouringParams &) -> VertexColouringResult
{
    VertexColouringResult result;
    result.n_colours = graph.size();

    for (int i = 0 ; i < graph.size() ; ++i)
        result.colouring.emplace(i, i);

    return result;
}
