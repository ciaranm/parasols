/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/cco_max_clique.hh>
#include <max_clique/print_incumbent.hh>
#include <graph/bit_graph.hh>
#include <graph/degree_sort.hh>
#include <graph/min_width_sort.hh>

#include <algorithm>

using namespace parasols;

namespace
{
    template <CCOPermutations perm_>
    struct SelectCCOPerm;

    template <>
    struct SelectCCOPerm<CCOPermutations::None>
    {
        template <unsigned size_>
        static auto colour_class_order(
                const FixedBitGraph<size_> & graph,
                const FixedBitSet<size_> & p,
                std::array<unsigned, size_ * bits_per_word> & p_order,
                std::array<unsigned, size_ * bits_per_word> & result) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            unsigned colour = 0;           // current colour
            unsigned i = 0;                // position in result

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
    };

    template <>
    struct SelectCCOPerm<CCOPermutations::Defer1>
    {
        template <unsigned size_>
        static auto colour_class_order(
                const FixedBitGraph<size_> & graph,
                const FixedBitSet<size_> & p,
                std::array<unsigned, size_ * bits_per_word> & p_order,
                std::array<unsigned, size_ * bits_per_word> & result) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            unsigned colour = 0;           // current colour
            unsigned i = 0;                // position in result

            unsigned d = 0;                // number deferred
            std::array<unsigned, size_ * bits_per_word> defer;

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<size_> q = p_left;

                // while we can still give something this colour
                unsigned number_with_this_colour = 0;
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
                    ++number_with_this_colour;
                }

                if (1 == number_with_this_colour) {
                    --i;
                    --colour;
                    defer[d++] = p_order[i];
                }
            }

            for (unsigned n = 0 ; n < d ; ++n) {
                ++colour;
                p_order[i] = defer[n];
                result[i] = colour;
                i++;
            }
        }
    };

    template <>
    struct SelectCCOPerm<CCOPermutations::Defer2>
    {
        template <unsigned size_>
        static auto colour_class_order(
                const FixedBitGraph<size_> & graph,
                const FixedBitSet<size_> & p,
                std::array<unsigned, size_ * bits_per_word> & p_order,
                std::array<unsigned, size_ * bits_per_word> & result) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            unsigned colour = 0;           // current colour
            unsigned i = 0;                // position in result

            unsigned d1 = 0, d2 = 0;       // number deferred
            std::array<unsigned, size_ * bits_per_word> defer1, defer2;

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<size_> q = p_left;

                // while we can still give something this colour
                unsigned number_with_this_colour = 0;
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
                    ++number_with_this_colour;
                }

                if (1 == number_with_this_colour) {
                    --colour;
                    defer1[d1++] = p_order[--i];
                }
                else if (2 == number_with_this_colour) {
                    --colour;
                    defer2[d2++] = p_order[--i];
                    defer2[d2++] = p_order[--i];
                }
            }

            for (unsigned n = 0 ; n < d2 ; n += 2) {
                ++colour;
                p_order[i] = defer2[n];
                result[i] = colour;
                i++;
                p_order[i] = defer2[n + 1];
                result[i] = colour;
                i++;
            }

            for (unsigned n = 0 ; n < d1 ; ++n) {
                ++colour;
                p_order[i] = defer1[n];
                result[i] = colour;
                i++;
            }
        }
    };

    template <>
    struct SelectCCOPerm<CCOPermutations::Sort>
    {
        template <unsigned size_>
        static auto colour_class_order(
                const FixedBitGraph<size_> & graph,
                const FixedBitSet<size_> & p,
                std::array<unsigned, size_ * bits_per_word> & p_order,
                std::array<unsigned, size_ * bits_per_word> & result) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            unsigned colour = 0;           // current colour
            unsigned i = 0;                // position in result

            // this is sloooooow. is it worth making it fast, or is d1 nearly as
            // good anyway?
            std::list<std::vector<unsigned> > colour_classes;

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                colour_classes.emplace_back();

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
                    colour_classes.back().push_back(v);
                }
            }

            // this is stable
            colour_classes.sort([] (const std::vector<unsigned> & a, const std::vector<unsigned> & b) -> bool {
                    return a.size() > b.size();
                    });

            for (auto & colour_class : colour_classes) {
                ++colour;
                for (auto & v : colour_class) {
                    result[i] = colour;
                    p_order[i] = v;
                    ++i;
                }
            }
        }
    };

    template <CCOPermutations perm_, unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,                      // vertex ordering
            FixedBitSet<size_> & c,                          // current candidate clique
            FixedBitSet<size_> & p,                          // potential additions
            MaxCliqueResult & result,
            const MaxCliqueParams & params,
            std::vector<int> & position
            ) -> void
    {
        ++result.nodes;

        auto c_popcount = c.popcount();

        // get our coloured vertices
        std::array<unsigned, size_ * bits_per_word> p_order, colours;
        SelectCCOPerm<perm_>::template colour_class_order<size_>(graph, p, p_order, colours);

        // for each v in p... (v comes later)
        for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
            ++position.back();

            // bound, timeout or early exit?
            if (c_popcount + colours[n] <= result.size || result.size >= params.stop_after_finding || params.abort.load())
                return;

            auto v = p_order[n];

            // consider taking v
            c.set(v);
            ++c_popcount;

            // filter p to contain vertices adjacent to v
            FixedBitSet<size_> new_p = p;
            graph.intersect_with_row(v, new_p);

            if (new_p.empty()) {
                // potential new best
                if (c_popcount > result.size) {
                    if (params.enumerate) {
                        ++result.result_count;
                        result.size = c_popcount - 1;
                    }
                    else
                        result.size = c_popcount;

                    result.members.clear();
                    for (int i = 0 ; i < graph.size() ; ++i)
                        if (c.test(i))
                            result.members.insert(o[i]);

                    print_incumbent(params, c_popcount, position);
                }
            }
            else {
                position.push_back(0);
                expand<perm_, size_>(graph, o, c, new_p, result, params, position);
                position.pop_back();
            }

            // now consider not taking v
            c.unset(v);
            p.unset(v);
            --c_popcount;
        }
    }

    template <CCOPermutations perm_, MaxCliqueOrder order_, unsigned size_>
    auto cco(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
    {
        MaxCliqueResult result;
        result.size = params.initial_bound;

        std::vector<int> o(graph.size()); // vertex ordering

        FixedBitSet<size_> c; // current candidate clique
        c.resize(graph.size());

        FixedBitSet<size_> p; // potential additions
        p.resize(graph.size());
        p.set_all();

        // populate our order with every vertex initially
        std::iota(o.begin(), o.end(), 0);

        switch (order_) {
            case MaxCliqueOrder::Degree:
                degree_sort(graph, o, false);
                break;
            case MaxCliqueOrder::MinWidth:
                min_width_sort(graph, o, false);
                break;
            case MaxCliqueOrder::ExDegree:
                exdegree_sort(graph, o, false);
                break;
            case MaxCliqueOrder::DynExDegree:
                dynexdegree_sort(graph, o, false);
                break;
        }

        // re-encode graph as a bit graph
        FixedBitGraph<size_> bit_graph;
        bit_graph.resize(graph.size());

        for (int i = 0 ; i < graph.size() ; ++i)
            for (int j = 0 ; j < graph.size() ; ++j)
                if (graph.adjacent(o[i], o[j]))
                    bit_graph.add_edge(i, j);

        std::vector<int> positions;
        positions.reserve(graph.size());
        positions.push_back(0);

        // go!
        expand<perm_, size_>(bit_graph, o, c, p, result, params, positions);

        // hack for enumerate
        if (params.enumerate)
            result.size = result.members.size();

        return result;
    }
}

template <CCOPermutations perm_, MaxCliqueOrder order_>
auto parasols::cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 1024, "Need to update here if max_graph_size is changed.");
    if (graph.size() < bits_per_word)
        return cco<perm_, order_, 1>(graph, params);
    else if (graph.size() < 2 * bits_per_word)
        return cco<perm_, order_, 2>(graph, params);
    else if (graph.size() < 4 * bits_per_word)
        return cco<perm_, order_, 4>(graph, params);
    else if (graph.size() < 8 * bits_per_word)
        return cco<perm_, order_, 8>(graph, params);
    else if (graph.size() < 16 * bits_per_word)
        return cco<perm_, order_, 16>(graph, params);
    else if (graph.size() < 32 * bits_per_word)
        return cco<perm_, order_, 32>(graph, params);
    else if (graph.size() < 64 * bits_per_word)
        return cco<perm_, order_, 64>(graph, params);
    else if (graph.size() < 128 * bits_per_word)
        return cco<perm_, order_, 128>(graph, params);
    else if (graph.size() < 256 * bits_per_word)
        return cco<perm_, order_, 256>(graph, params);
    else if (graph.size() < 512 * bits_per_word)
        return cco<perm_, order_, 512>(graph, params);
    else if (graph.size() < 1024 * bits_per_word)
        return cco<perm_, order_, 1024>(graph, params);
    else
        throw GraphTooBig();
}

template auto parasols::cco_max_clique<CCOPermutations::None, MaxCliqueOrder::Degree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1, MaxCliqueOrder::Degree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer2, MaxCliqueOrder::Degree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort, MaxCliqueOrder::Degree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::cco_max_clique<CCOPermutations::None, MaxCliqueOrder::MinWidth>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1, MaxCliqueOrder::MinWidth>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer2, MaxCliqueOrder::MinWidth>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort, MaxCliqueOrder::MinWidth>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::cco_max_clique<CCOPermutations::None, MaxCliqueOrder::ExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1, MaxCliqueOrder::ExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer2, MaxCliqueOrder::ExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort, MaxCliqueOrder::ExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::cco_max_clique<CCOPermutations::None, MaxCliqueOrder::DynExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1, MaxCliqueOrder::DynExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer2, MaxCliqueOrder::DynExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort, MaxCliqueOrder::DynExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

