/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/add_dominated_vertices.hh>
#include <random>

using namespace parasols;

Graph parasols::add_dominated_vertices(const Graph & graph, int n, double p, double q, int seed)
{
    std::mt19937 rand;
    rand.seed(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::uniform_int_distribution<int> vertex_dist(0, graph.size());

    Graph result(graph.size() + n, graph.add_one_for_output());

    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (graph.adjacent(i, j))
                result.add_edge(i, j);

    for (int v = 0 ; v < n ; ++v) {
        int clone = vertex_dist(rand);

        if (dist(rand) <= q)
            result.add_edge(graph.size() + v, clone);

        for (int i = 0 ; i < graph.size() ; ++i)
            if (graph.adjacent(clone, i) && dist(rand) <= p)
                result.add_edge(graph.size() + v, i);
    }

    return result;
}

