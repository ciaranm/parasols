/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <vertex_colour/naive_vertex_colour.hh>

#include <graph/degree_sort.hh>

#include <algorithm>

using namespace parasols;

namespace
{
    auto legal(
            const Graph & graph,
            const std::vector<int> & order,
            const std::vector<int> & current,
            const int vertex,
            const int candidate_k
            ) -> bool
    {
        for (int v = 0 ; v < graph.size() ; ++v)
            if (current[order[v]] == candidate_k && graph.adjacent(order[v], order[vertex]))
                return false;

        return true;
    }

    auto expand(
            const Graph & graph,
            const VertexColouringParams & params,
            std::vector<int> & order,
            std::vector<int> & current,
            int vertex,
            int k_max,
            int & best_k,
            std::vector<int> & best,
            unsigned long long & nodes
            ) -> void
    {
        ++nodes;

        if (params.abort->load())
            return;

        if (vertex == graph.size()) {
            if (k_max < best_k) {
                best_k = k_max;
                best = current;
            }

            return;
        }

        for (int k = 1 ; k <= k_max + 1 ; ++k) {
            if (legal(graph, order, current, vertex, k)) {
                current[order[vertex]] = k;
                expand(graph, params, order, current, vertex + 1, std::max(k, k_max), best_k, best, nodes);
                current[order[vertex]] = 0;
            }
        }
    }
}

auto parasols::naive_vertex_colour(const Graph & graph, const VertexColouringParams & params) -> VertexColouringResult
{
    VertexColouringResult result;

    if (0 == params.initial_bound)
        result.colours = graph.size() + 1;
    else
        result.colours = params.initial_bound;
    result.colouring.resize(graph.size());

    std::vector<int> current((graph.size()));

    std::vector<int> order((graph.size()));
    std::iota(order.begin(), order.end(), 0);
    dynexdegree_sort(graph, order, false);

    expand(graph, params, order, current, 0, 0, result.colours, result.colouring, result.nodes);

    return result;
}


