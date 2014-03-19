/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/product.hh>

auto parasols::modular_product(const Graph & g1, const Graph & g2) -> Graph
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

auto parasols::unproduct(const Graph & g1, const Graph &, int v) -> std::pair<int, int>
{
    return std::make_pair(v % g1.size(), v / g1.size());
}

