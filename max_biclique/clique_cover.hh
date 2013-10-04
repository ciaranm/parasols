/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_BICLIQUE_CLIQUE_COVER_HH
#define PARASOLS_GUARD_MAX_BICLIQUE_CLIQUE_COVER_HH 1

#include <graph/bit_graph.hh>
#include <array>

namespace parasols
{
    /* This is the complement of San Segundo's variation of Tomita's max clique
     * colour bound: we can't grow a side by more than the size of a maximum
     * independent set in the remaining addable vertices. Independent set is
     * NP-hard, but we can get a bound on it using greedy clique cover (the
     * dual of colouring). */
    template <unsigned size_>
    auto clique_cover(
            const FixedBitGraph<size_> & graph,
            const FixedBitSet<size_> & p,
            std::array<unsigned, size_ * bits_per_word> & p_order,
            std::array<unsigned, size_ * bits_per_word> & result) -> void;
}

#endif
