/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <vertex_colour/fc_vertex_colour.hh>
#include <graph/degree_sort.hh>

#include <algorithm>

using namespace parasols;

namespace
{
    auto expand(
            const Graph & graph,
            const VertexColouringParams & params,
            std::vector<int> & order,
            std::vector<int> & current,
            std::vector<std::vector<bool> > & allowed,
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

        if (vertex == graph.size()) {
            if (k_max < best_k) {
                best_k = k_max;
                best = current;
            }

            return;
        }

        for (int k = 1 ; k <= k_max + 1 ; ++k) {
            if (k_max >= best_k)
                break;

            if (allowed[k - 1][vertex]) {
                current[order[vertex]] = k;
                auto save_allowed_km1 = allowed[k - 1];

                for (int a = 0 ; a < graph.size() ; ++a)
                    if (graph.adjacent(order[vertex], order[a])) {
                        if (allowed[k - 1][a]) {
                            allowed[k - 1][a] = false;
                            bool any = false;
                            for (int c = 0 ; c < best_k ; ++c)
                                if (allowed[c][a]) {
                                    any = true;
                                    break;
                                }
                            if (! any) {
                                allowed[k - 1] = save_allowed_km1;
                                return;
                            }
                        }
                    }

                expand(graph, params, order, current, allowed, vertex + 1, std::max(k, k_max), best_k, best, nodes);
                allowed[k - 1] = save_allowed_km1;
                current[order[vertex]] = 0;
            }
        }
    }
}

auto parasols::fc_vertex_colour(const Graph & graph, const VertexColouringParams & params) -> VertexColouringResult
{
    VertexColouringResult result;

    if (0 == params.initial_bound)
        result.colours = graph.size() + 1;
    else
        result.colours = params.initial_bound;

    result.colouring.resize(graph.size());

    std::vector<int> current((graph.size()));

    std::vector<std::vector<bool> > allowed((graph.size()));
    for (auto & a : allowed) {
        a.resize(graph.size());
        std::fill(a.begin(), a.end(), true);
    }

    std::vector<int> order((graph.size()));
    std::iota(order.begin(), order.end(), 0);
    dynexdegree_sort(graph, order, false);

    expand(graph, params, order, current, allowed, 0, 0, result.colours, result.colouring, result.nodes);

    return result;
}


