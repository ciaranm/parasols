/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <vertex_colour/naive_vertex_colour.hh>

using namespace parasols;

namespace
{
    auto legal(
            const Graph & graph,
            const std::vector<int> & current,
            const int vertex,
            const int candidate_k
            ) -> bool
    {
        for (int v = 0 ; v < graph.size() ; ++v)
            if (current[v] == candidate_k && graph.adjacent(v, vertex))
                return false;

        return true;
    }

    auto expand(
            const Graph & graph,
            const VertexColouringParams & params,
            std::vector<int> & current,
            int vertex,
            int k_max,
            int & best_k,
            std::vector<int> & best,
            unsigned long long & nodes
            ) -> void
    {
        ++nodes;

        if (params.abort.load())
            return;

        if (vertex == graph.size() + 1) {
            if (k_max < best_k) {
                best_k = k_max;
                best = current;
            }

            return;
        }

        for (int k = 1 ; k <= k_max + 1 ; ++k) {
            if (legal(graph, current, vertex, k)) {
                current[vertex] = k;
                expand(graph, params, current, vertex + 1, std::max(k, k_max), best_k, best, nodes);
                current[vertex] = 0;
            }
        }
    }
}

auto parasols::naive_vertex_colour(const Graph & graph, const VertexColouringParams & params) -> VertexColouringResult
{
    VertexColouringResult result;
    result.colours = graph.size() + 1;
    result.colouring.resize(graph.size());

    std::vector<int> current((graph.size()));

    expand(graph, params, current, 0, 0, result.colours, result.colouring, result.nodes);

    return result;
}


