/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_biclique/clique_cover.hh>

using namespace parasols;

template <unsigned size_>
auto parasols::clique_cover(
        const FixedBitGraph<size_> & graph,
        const FixedBitSet<size_> & p,
        std::array<unsigned, size_ * bits_per_word> & p_order,
        std::array<unsigned, size_ * bits_per_word> & result) -> void
{
    FixedBitSet<size_> p_left = p; // not cliqued yet
    int clique = 0;                // current clique
    int i = 0;                     // position in result

    // while we've things left to clique
    while (! p_left.empty()) {
        // next clique
        ++clique;
        // things that can still be given this clique
        FixedBitSet<size_> q = p_left;

        // while we can still give something this clique
        while (! q.empty()) {
            // first thing we can clique
            int v = q.first_set_bit();
            p_left.unset(v);
            q.unset(v);

            // can't give anything nonadjacent to this the same clique
            graph.intersect_with_row(v, q);

            // record in result
            result[i] = clique;
            p_order[i] = v;
            ++i;
        }
    }
}

static_assert(max_graph_words == 256, "Need to update here if max_graph_size is changed.");
template auto parasols::clique_cover(const FixedBitGraph<1> & graph, const FixedBitSet<1> & p, std::array<unsigned, 1 * bits_per_word> & p_order,
        std::array<unsigned, 1 * bits_per_word> & result) -> void;
template auto parasols::clique_cover(const FixedBitGraph<2> & graph, const FixedBitSet<2> & p, std::array<unsigned, 2 * bits_per_word> & p_order,
        std::array<unsigned, 2 * bits_per_word> & result) -> void;
template auto parasols::clique_cover(const FixedBitGraph<4> & graph, const FixedBitSet<4> & p, std::array<unsigned, 4 * bits_per_word> & p_order,
        std::array<unsigned, 4 * bits_per_word> & result) -> void;
template auto parasols::clique_cover(const FixedBitGraph<8> & graph, const FixedBitSet<8> & p, std::array<unsigned, 8 * bits_per_word> & p_order,
        std::array<unsigned, 8 * bits_per_word> & result) -> void;
template auto parasols::clique_cover(const FixedBitGraph<16> & graph, const FixedBitSet<16> & p, std::array<unsigned, 16 * bits_per_word> & p_order,
        std::array<unsigned, 16 * bits_per_word> & result) -> void;
template auto parasols::clique_cover(const FixedBitGraph<32> & graph, const FixedBitSet<32> & p, std::array<unsigned, 32 * bits_per_word> & p_order,
        std::array<unsigned, 32 * bits_per_word> & result) -> void;
template auto parasols::clique_cover(const FixedBitGraph<64> & graph, const FixedBitSet<64> & p, std::array<unsigned, 64 * bits_per_word> & p_order,
        std::array<unsigned, 64 * bits_per_word> & result) -> void;
template auto parasols::clique_cover(const FixedBitGraph<128> & graph, const FixedBitSet<128> & p, std::array<unsigned, 128 * bits_per_word> & p_order,
        std::array<unsigned, 128 * bits_per_word> & result) -> void;
template auto parasols::clique_cover(const FixedBitGraph<256> & graph, const FixedBitSet<256> & p, std::array<unsigned, 256 * bits_per_word> & p_order,
        std::array<unsigned, 256 * bits_per_word> & result) -> void;

