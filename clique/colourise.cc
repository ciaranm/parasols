/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <clique/colourise.hh>
#include <clique/graph.hh>
#include <clique/bit_graph.hh>

using namespace clique;

auto clique::colourise(
        const Graph & graph,
        Buckets & buckets,
        std::vector<int> & p,
        const std::vector<int> & o
        ) -> std::vector<unsigned>
{
    auto result = std::vector<unsigned>{ };
    result.resize(o.size());

    for (auto & bucket : buckets)
        bucket.clear();

    // for each vertex, in order...
    for (auto v : o) {
        // find it an appropriate bucket
        auto bucket = buckets.begin();
        for ( ; ; ++bucket) {
            // is this bucket any good?
            auto conflicts = false;
            for (auto & w : *bucket) {
                if (graph.adjacent(v, w)) {
                    conflicts = true;
                    break;
                }
            }
            if (! conflicts)
                break;
        }
        bucket->push_back(v);
    }

    // now empty our buckets, in turn, into the result.
    p.clear();
    auto i = 0;
    for (auto bucket = buckets.begin(), bucket_end = buckets.end() ; bucket != bucket_end ; ++bucket)
        for (const auto & v : *bucket) {
            p.push_back(v);
            result[i++] = bucket - buckets.begin() + 1;
        }

    return result;
}

auto clique::make_buckets(const int size) -> Buckets
{
    Buckets buckets;
    buckets.resize(size);
    for (auto & bucket : buckets)
        bucket.reserve(size);
    return std::move(buckets);
}

template <unsigned size_>
auto clique::colourise(
        const FixedBitGraph<size_> & graph,
        const FixedBitSet<size_> & p,
        std::array<unsigned, size_ * bits_per_word> & p_order,
        std::array<unsigned, size_ * bits_per_word> & result) -> void
{
    FixedBitSet<size_> p_left = p; // not coloured yet
    int colour = 0;                // current colour
    int i = 0;                     // position in result

    // while we've things left to colour
    while (! p_left.empty()) {
        // next colour
        ++colour;
        // things that can still be given this colour
        FixedBitSet<size_> q = p_left;

        // while we can still give something this colour
        while (! q.empty()) {
            // first thing we can colour
            int v = q.first_set_bit();
            p_left.unset(v);
            q.unset(v);

            // can't give anything adjacent to this the same colour
            graph.intersect_with_row_complement(v, q);

            // record in result
            result[i] = colour;
            p_order[i] = v;
            ++i;
        }
    }
}

static_assert(max_graph_words == 256, "Need to update here if max_graph_size is changed.");
template auto clique::colourise(const FixedBitGraph<1> & graph, const FixedBitSet<1> & p, std::array<unsigned, 1 * bits_per_word> & p_order,
        std::array<unsigned, 1 * bits_per_word> & result) -> void;
template auto clique::colourise(const FixedBitGraph<2> & graph, const FixedBitSet<2> & p, std::array<unsigned, 2 * bits_per_word> & p_order,
        std::array<unsigned, 2 * bits_per_word> & result) -> void;
template auto clique::colourise(const FixedBitGraph<4> & graph, const FixedBitSet<4> & p, std::array<unsigned, 4 * bits_per_word> & p_order,
        std::array<unsigned, 4 * bits_per_word> & result) -> void;
template auto clique::colourise(const FixedBitGraph<8> & graph, const FixedBitSet<8> & p, std::array<unsigned, 8 * bits_per_word> & p_order,
        std::array<unsigned, 8 * bits_per_word> & result) -> void;
template auto clique::colourise(const FixedBitGraph<16> & graph, const FixedBitSet<16> & p, std::array<unsigned, 16 * bits_per_word> & p_order,
        std::array<unsigned, 16 * bits_per_word> & result) -> void;
template auto clique::colourise(const FixedBitGraph<32> & graph, const FixedBitSet<32> & p, std::array<unsigned, 32 * bits_per_word> & p_order,
        std::array<unsigned, 32 * bits_per_word> & result) -> void;
template auto clique::colourise(const FixedBitGraph<64> & graph, const FixedBitSet<64> & p, std::array<unsigned, 64 * bits_per_word> & p_order,
        std::array<unsigned, 64 * bits_per_word> & result) -> void;
template auto clique::colourise(const FixedBitGraph<128> & graph, const FixedBitSet<128> & p, std::array<unsigned, 128 * bits_per_word> & p_order,
        std::array<unsigned, 128 * bits_per_word> & result) -> void;
template auto clique::colourise(const FixedBitGraph<256> & graph, const FixedBitSet<256> & p, std::array<unsigned, 256 * bits_per_word> & p_order,
        std::array<unsigned, 256 * bits_per_word> & result) -> void;

