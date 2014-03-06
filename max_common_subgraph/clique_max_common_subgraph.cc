/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_common_subgraph/clique_max_common_subgraph.hh>

using namespace parasols;

namespace
{
    Graph modular_product(const Graph & g1, const Graph & g2)
    {
        Graph result(g1.size() * g2.size(), false);

        for (int v1 = 0 ; v1 < g1.size() ; ++v1)
            for (int v2 = 0 ; v2 < g2.size() ; ++v2)
                for (int w1 = 0 ; w1 < g1.size() ; ++w1)
                    for (int w2 = 0 ; w2 < g2.size() ; ++w2) {
                        unsigned p1 = v2 * g1.size() + v1;
                        unsigned p2 = w2 * g1.size() + w1;
                        if (p1 != p2 && v1 != w1 && v2 != w2 && (g1.adjacent(v1, w1) == g2.adjacent(v2, w2))) {
                            result.add_edge(p1, p2);
                        }
                    }
        return result;
    }
}

auto parasols::clique_max_common_subgraph(
        const std::pair<Graph, Graph> & graphs,
        const MaxCommonSubgraphParams & params) -> MaxCommonSubgraphResult
{
    MaxCliqueParams clique_params;
    clique_params.initial_bound = params.initial_bound;
    clique_params.stop_after_finding = params.stop_after_finding;
    clique_params.n_threads = params.n_threads;
    clique_params.print_incumbents = params.print_incumbents;
    clique_params.start_time = params.start_time;
    clique_params.order_function = params.order_function;
    clique_params.abort.store(false);
    // abort?

    auto clique_result = params.max_clique_algorithm(modular_product(graphs.first, graphs.second), clique_params);

    MaxCommonSubgraphResult result;
    result.size = clique_result.size;
    result.nodes = clique_result.nodes;
    result.times = clique_result.times;

    for (auto & v : clique_result.members)
        result.isomorphism.emplace(v % graphs.first.size(), v / graphs.first.size());

    return result;
}

