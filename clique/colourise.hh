/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CLIQUE_GUARD_CLIQUE_COLOURISE_HH
#define CLIQUE_GUARD_CLIQUE_COLOURISE_HH 1

#include <clique/graph.hh>
#include <clique/bit_graph.hh>
#include <vector>

namespace clique
{
    using Buckets = std::vector<std::vector<int> >;

    /**
     * Allocate some buckets, for use by colourise.
     */
    auto make_buckets(const int size) -> Buckets;

    /**
     * Perform Tomita's colour-sorting step.
     */
    auto colourise(
            const Graph & graph,
            Buckets & buckets,
            std::vector<int> & p,
            const std::vector<int> & o) -> std::vector<unsigned>;

    /**
     * Perform San Segundo's bitset version of colour sorting.
     */
    template <unsigned size_>
    auto colourise(
            const FixedBitGraph<size_> & graph,
            const FixedBitSet<size_> & p,
            std::array<unsigned, size_ * bits_per_word> & p_order,
            std::array<unsigned, size_ * bits_per_word> & result) -> void;
}

#endif
