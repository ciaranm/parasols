/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/cco_max_clique.hh>
#include <max_clique/print_incumbent.hh>

#include <graph/bit_graph.hh>

#include <algorithm>

#ifdef DISTRIBUTION_INSTRUMENTATION
#  include <iostream>
#endif

using namespace parasols;

namespace
{
#ifdef DISTRIBUTION_INSTRUMENTATION
    struct DistributionCounter
    {
        std::vector<std::vector<double> > data;

        ~DistributionCounter()
        {
            for (unsigned i = 0 ; i < data.size() ; ++i) {
                unsigned total = 0;
                std::vector<unsigned> buckets((21));
                for (auto & d : data.at(i)) {
                    ++total;
                    ++buckets.at(int((d + 1) * 10));
                }

                for (int b = 0 ; b < 21 ; ++b)
                    if (0 == total)
                        std::cerr << i << " " << ((b - 10) / 10.0) << " " << 0.0 << std::endl;
                    else
                        std::cerr << i << " " << ((b - 10) / 10.0) << " " << ((0.0 + buckets.at(b)) / total) << std::endl;

                std::cerr << std::endl;
            }
        }

        void add(std::vector<std::pair<double, double> > & colour_class_sizes)
        {
            if (colour_class_sizes.size() < 2)
                return;

            // second is size
            std::stable_sort(colour_class_sizes.begin(), colour_class_sizes.end(), [] (
                        const std::pair<double, double> & a,
                        const std::pair<double, double> & b) {
                    return a.second > b.second;
                    });

            // second is actual rank
            for (unsigned i = 0 ; i < colour_class_sizes.size() ; ) {
                unsigned j = i;
                double v = (i + 1);
                while (j + 1 < colour_class_sizes.size() && colour_class_sizes.at(j + 1) == colour_class_sizes.at(i)) {
                    ++j;
                    v += (j + 1);
                }

                v /= (j - i + 1);
                for ( ; i <= j ; ++i)
                    colour_class_sizes.at(i).second = v;
            }

            double first_mean = 0.0, second_mean = 0.0;
            for (auto & c : colour_class_sizes) {
                first_mean += c.first;
                second_mean += c.second;
            }
            first_mean /= colour_class_sizes.size();
            second_mean /= colour_class_sizes.size();

            double top = 0.0;
            for (auto & c : colour_class_sizes)
                top += (c.first - first_mean) * (c.second - second_mean);

            double bottom_first = 0.0, bottom_second = 0.0;
            for (auto & c : colour_class_sizes) {
                bottom_first += (c.first - first_mean) * (c.first - first_mean);
                bottom_second += (c.second - second_mean) * (c.second - second_mean);
            }

            double rho = top / sqrt(bottom_first * bottom_second);

            if (data.size() < colour_class_sizes.size() + 1)
                data.resize(colour_class_sizes.size() + 1);
            data.at(colour_class_sizes.size()).push_back(rho);
        }
    };

    static DistributionCounter distribution_counter;
#endif

    template <CCOPermutations perm_>
    struct SelectColourClassOrderOverload
    {
    };

    template <CCOPermutations perm_, unsigned size_>
    struct CCOBase
    {
        FixedBitGraph<size_> graph;
        const MaxCliqueParams & params;
        MaxCliqueResult result;
        std::vector<int> order;

        CCOBase(const Graph & g, const MaxCliqueParams & p) :
            params(p),
            order(g.size())
        {
            result.size = params.initial_bound;

            // populate our order with every vertex initially
            std::iota(order.begin(), order.end(), 0);
            params.order_function(g, order);

            // re-encode graph as a bit graph
            graph.resize(g.size());

            for (int i = 0 ; i < g.size() ; ++i)
                for (int j = 0 ; j < g.size() ; ++j)
                    if (g.adjacent(order[i], order[j]))
                        graph.add_edge(i, j);
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::None> &,
                const FixedBitSet<size_> & p,
                std::array<unsigned, size_ * bits_per_word> & p_order,
                std::array<unsigned, size_ * bits_per_word> & result) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            unsigned colour = 0;           // current colour
            unsigned i = 0;                // position in result

#ifdef DISTRIBUTION_INSTRUMENTATION
            std::vector<std::pair<double, double> > colour_class_sizes;
#endif

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<size_> q = p_left;

                // while we can still give something this colour
#ifdef DISTRIBUTION_INSTRUMENTATION
                unsigned number_with_this_colour = 0;
#endif
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
#ifdef DISTRIBUTION_INSTRUMENTATION
                    ++number_with_this_colour;
#endif
                }

#ifdef DISTRIBUTION_INSTRUMENTATION
                colour_class_sizes.push_back(std::make_pair(colour_class_sizes.size() + 1, number_with_this_colour));
#endif
            }

#ifdef DISTRIBUTION_INSTRUMENTATION
            distribution_counter.add(colour_class_sizes);
#endif
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::Defer1> &,
                const FixedBitSet<size_> & p,
                std::array<unsigned, size_ * bits_per_word> & p_order,
                std::array<unsigned, size_ * bits_per_word> & result) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            unsigned colour = 0;           // current colour
            unsigned i = 0;                // position in result

            unsigned d = 0;                // number deferred
            std::array<unsigned, size_ * bits_per_word> defer;

#ifdef DISTRIBUTION_INSTRUMENTATION
            std::vector<std::pair<double, double> > colour_class_sizes;
#endif

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
                else {
#ifdef DISTRIBUTION_INSTRUMENTATION
                    colour_class_sizes.push_back(std::make_pair(colour_class_sizes.size() + 1, number_with_this_colour));
#endif
                }
            }

            for (unsigned n = 0 ; n < d ; ++n) {
                ++colour;
                p_order[i] = defer[n];
                result[i] = colour;
                i++;

#ifdef DISTRIBUTION_INSTRUMENTATION
                colour_class_sizes.push_back(std::make_pair(colour_class_sizes.size() + 1, 1));
#endif
            }

#ifdef DISTRIBUTION_INSTRUMENTATION
            distribution_counter.add(colour_class_sizes);
#endif
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::Sort> &,
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

#ifdef DISTRIBUTION_INSTRUMENTATION
            std::vector<std::pair<double, double> > colour_class_sizes;
#endif

            for (auto & colour_class : colour_classes) {
                ++colour;
                for (auto & v : colour_class) {
                    result[i] = colour;
                    p_order[i] = v;
                    ++i;
                }
#ifdef DISTRIBUTION_INSTRUMENTATION
                colour_class_sizes.push_back(std::make_pair(colour_class_sizes.size() + 1, colour_class.size()));
#endif
            }

#ifdef DISTRIBUTION_INSTRUMENTATION
            distribution_counter.add(colour_class_sizes);
#endif
        }
    };

    template <CCOPermutations perm_, unsigned size_>
    struct CCO : CCOBase<perm_, size_>
    {
        using CCOBase<perm_, size_>::CCOBase;

        using CCOBase<perm_, size_>::graph;
        using CCOBase<perm_, size_>::order;
        using CCOBase<perm_, size_>::params;
        using CCOBase<perm_, size_>::result;
        using CCOBase<perm_, size_>::colour_class_order;

        auto run() -> void
        {
            FixedBitSet<size_> c; // current candidate clique
            c.resize(graph.size());

            FixedBitSet<size_> p; // potential additions
            p.resize(graph.size());
            p.set_all();

            std::vector<int> positions;
            positions.reserve(graph.size());
            positions.push_back(0);

            // go!
            expand(c, p, positions);

            // hack for enumerate
            if (params.enumerate)
                result.size = result.members.size();
        }

        auto expand(
                FixedBitSet<size_> & c,                          // current candidate clique
                FixedBitSet<size_> & p,                          // potential additions
                std::vector<int> & position
                ) -> void
        {
            ++result.nodes;

            auto c_popcount = c.popcount();

            // get our coloured vertices
            std::array<unsigned, size_ * bits_per_word> p_order, colours;
            colour_class_order(SelectColourClassOrderOverload<perm_>(), p, p_order, colours);

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
                                result.members.insert(order[i]);

                        print_incumbent(params, c_popcount, position);
                    }
                }
                else {
                    position.push_back(0);
                    expand(c, new_p, position);
                    position.pop_back();
                }

                // now consider not taking v
                c.unset(v);
                p.unset(v);
                --c_popcount;
            }
        }
    };

    template <unsigned...>
    struct GraphSizes;

    struct NoMoreGraphSizes
    {
    };

    template <unsigned n_, unsigned... n_rest_>
    struct GraphSizes<n_, n_rest_...>
    {
        enum { n = n_ };

        using Rest = GraphSizes<n_rest_...>;
    };

    template <unsigned n_>
    struct GraphSizes<n_>
    {
        enum { n = n_ };

        using Rest = NoMoreGraphSizes;
    };

    template <template <unsigned> class Algorithm_, typename Result_, unsigned... sizes_, typename... Params_>
    auto select_graph_size(const GraphSizes<sizes_...> &, const Graph & graph, const Params_ & ... params) -> Result_
    {
        if (graph.size() < GraphSizes<sizes_...>::n * bits_per_word) {
            Algorithm_<GraphSizes<sizes_...>::n> algorithm{ graph, params... };
            algorithm.run();
            return algorithm.result;
        }
        else
            return select_graph_size<Algorithm_, Result_>(typename GraphSizes<sizes_...>::Rest(), graph, params...);
    }

    template <template <unsigned> class Algorithm_, typename Result_, typename... Params_>
    auto select_graph_size(const NoMoreGraphSizes &, const Graph &, const Params_ & ...) -> Result_
    {
        throw GraphTooBig();
    }

    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 1024, "Need to update here if max_graph_size is changed.");

    using AllGraphSizes = GraphSizes<1, 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128, 256, 512, 1024>;

    template <CCOPermutations perm_>
    struct Apply
    {
        template <unsigned size_> using SelectCCO = CCO<perm_, size_>;
    };
}

template <CCOPermutations perm_>
auto parasols::cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<Apply<perm_>::template SelectCCO, MaxCliqueResult>(AllGraphSizes(), graph, params);
}

template auto parasols::cco_max_clique<CCOPermutations::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

