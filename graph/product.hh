/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_PRODUCT_HH
#define PARASOLS_GUARD_GRAPH_PRODUCT_HH 1

#include <graph/graph.hh>
#include <utility>

namespace parasols
{
    auto modular_product(const Graph & g1, const Graph & g2) -> Graph;

    auto subgraph_modular_product(const Graph & g1, const Graph & g2) -> Graph;

    auto unproduct(const Graph & g1, const Graph & g2, int v) -> std::pair<int, int>;
}

#endif
