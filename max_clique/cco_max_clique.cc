/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/cco_max_clique.hh>
#include <max_clique/print_incumbent.hh>

#include <graph/bit_graph.hh>

#include <threads/queue.hh>
#include <threads/atomic_incumbent.hh>

#include <algorithm>
#include <thread>
#include <mutex>

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

    template <CCOPermutations perm_, unsigned size_, typename ActualType_>
    struct CCOBase
    {
        FixedBitGraph<size_> graph;
        const MaxCliqueParams & params;
        std::vector<int> order;

        CCOBase(const Graph & g, const MaxCliqueParams & p) :
            params(p),
            order(g.size())
        {
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

        template <typename... MoreArgs_>
        auto expand(
                FixedBitSet<size_> & c,                          // current candidate clique
                FixedBitSet<size_> & p,                          // potential additions
                std::vector<int> & position,
                MoreArgs_ && ... more_args_
                ) -> void
        {
            static_cast<ActualType_ *>(this)->incremement_nodes(std::forward<MoreArgs_>(more_args_)...);

            auto c_popcount = c.popcount();

            // get our coloured vertices
            std::array<unsigned, size_ * bits_per_word> p_order, colours;
            colour_class_order(SelectColourClassOrderOverload<perm_>(), p, p_order, colours);

            // for each v in p... (v comes later)
            for (int n = p.popcount() - 1 ; n >= 0 ; --n) {
                ++position.back();

                // bound, timeout or early exit?
                unsigned best_anywhere_value = static_cast<ActualType_ *>(this)->get_best_anywhere_value();
                if (c_popcount + colours[n] <= best_anywhere_value || best_anywhere_value >= params.stop_after_finding || params.abort.load())
                    return;

                auto v = p_order[n];

                // consider taking v
                c.set(v);
                ++c_popcount;

                // filter p to contain vertices adjacent to v
                FixedBitSet<size_> new_p = p;
                graph.intersect_with_row(v, new_p);

                if (new_p.empty()) {
                    static_cast<ActualType_ *>(this)->potential_new_best(c_popcount, c, position, std::forward<MoreArgs_>(more_args_)...);
                }
                else {
                    position.push_back(0);
                    static_cast<ActualType_ *>(this)->recurse(c, new_p, position, c_popcount + colours[n], std::forward<MoreArgs_>(more_args_)...);
                    position.pop_back();
                }

                // now consider not taking v
                c.unset(v);
                p.unset(v);
                --c_popcount;
            }
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::None> &,
                const FixedBitSet<size_> & p,
                std::array<unsigned, size_ * bits_per_word> & p_order,
                std::array<unsigned, size_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            unsigned colour = 0;           // current colour
            unsigned i = 0;                // position in p_bounds

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
                    p_bounds[i] = colour;
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
                std::array<unsigned, size_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            unsigned colour = 0;           // current colour
            unsigned i = 0;                // position in p_bounds

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
                    p_bounds[i] = colour;
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
                p_bounds[i] = colour;
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
                std::array<unsigned, size_ * bits_per_word> & p_bounds) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            unsigned colour = 0;           // current colour
            unsigned i = 0;                // position in p_bounds

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

                    // record in p_bounds
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
                    p_bounds[i] = colour;
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
    struct CCO : CCOBase<perm_, size_, CCO<perm_, size_> >
    {
        using CCOBase<perm_, size_, CCO<perm_, size_> >::CCOBase;

        using CCOBase<perm_, size_, CCO<perm_, size_> >::graph;
        using CCOBase<perm_, size_, CCO<perm_, size_> >::params;
        using CCOBase<perm_, size_, CCO<perm_, size_> >::expand;
        using CCOBase<perm_, size_, CCO<perm_, size_> >::order;

        MaxCliqueResult result;

        auto run() -> MaxCliqueResult
        {
            result.size = params.initial_bound;

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

            return result;
        }

        auto incremement_nodes() -> void
        {
            ++result.nodes;
        }

        auto recurse(
                FixedBitSet<size_> & c,
                FixedBitSet<size_> & p,
                std::vector<int> & position,
                unsigned
                ) -> void
        {
            expand(c, p, position);
        }

        auto potential_new_best(
                unsigned c_popcount,
                const FixedBitSet<size_> & c,
                std::vector<int> & position) -> void
        {
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

        auto get_best_anywhere_value() -> unsigned
        {
            return result.size;
        }
    };

    template <unsigned size_>
    struct QueueItem
    {
        FixedBitSet<size_> c;
        FixedBitSet<size_> p;
        unsigned cn;
        std::vector<int> position;
    };

    template <CCOPermutations perm_, unsigned size_>
    struct TCCO : CCOBase<perm_, size_, TCCO<perm_, size_> >
    {
        using CCOBase<perm_, size_, TCCO<perm_, size_> >::CCOBase;

        using CCOBase<perm_, size_, TCCO<perm_, size_> >::graph;
        using CCOBase<perm_, size_, TCCO<perm_, size_> >::params;
        using CCOBase<perm_, size_, TCCO<perm_, size_> >::expand;
        using CCOBase<perm_, size_, TCCO<perm_, size_> >::order;

        AtomicIncumbent best_anywhere; // global incumbent

        auto run() -> MaxCliqueResult
        {
            best_anywhere.update(params.initial_bound);

            MaxCliqueResult global_result;
            global_result.size = params.initial_bound;
            std::mutex global_result_mutex;

            Queue<QueueItem<size_> > queue{ params.n_threads, false }; // work queue

            std::list<std::thread> threads; // populating thread, and workers

            /* populate */
            threads.push_back(std::thread([&] {
                        MaxCliqueResult local_result; // local result

                        FixedBitSet<size_> c; // local candidate clique
                        c.resize(graph.size());

                        FixedBitSet<size_> p; // local potential additions
                        p.resize(graph.size());
                        p.set_all();

                        std::vector<int> position;
                        position.reserve(graph.size());
                        position.push_back(0);

                        // populate!
                        expand(c, p, position, &queue, local_result);

                        // merge results
                        queue.initial_producer_done();
                        std::unique_lock<std::mutex> guard(global_result_mutex);
                        global_result.merge(local_result);
            }));

            /* workers */
            for (unsigned i = 0 ; i < params.n_threads ; ++i) {
                threads.push_back(std::thread([&, i] {
                            auto start_time = std::chrono::steady_clock::now(); // local start time

                            MaxCliqueResult local_result; // local result

                            while (true) {
                                // get some work to do
                                QueueItem<size_> args;
                                if (! queue.dequeue_blocking(args))
                                    break;

                                print_position(params, "dequeued", args.position);

                                // re-evaluate the bound against our new best
                                if (args.cn <= best_anywhere.get())
                                    continue;

                                // do some work
                                expand(args.c, args.p, args.position, nullptr, local_result);
                            }

                            auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);

                            // merge results
                            {
                                std::unique_lock<std::mutex> guard(global_result_mutex);
                                global_result.merge(local_result);
                                global_result.times.push_back(overall_time);
                            }
                            }));
            }

            // wait until they're done, and clean up threads
            for (auto & t : threads)
                t.join();

            return global_result;
        }

        auto incremement_nodes(
                Queue<QueueItem<size_> > * const,
                MaxCliqueResult & local_result) -> void
        {
            ++local_result.nodes;
        }

        auto recurse(
                FixedBitSet<size_> & c,
                FixedBitSet<size_> & p,
                std::vector<int> & position,
                unsigned cn,
                Queue<QueueItem<size_> > * const maybe_queue,
                MaxCliqueResult & local_result
                ) -> void
        {
            if (maybe_queue)
                maybe_queue->enqueue(QueueItem<size_>{ c, p, cn, position });
            else
                expand(c, p, position, nullptr, local_result);
        }

        auto potential_new_best(
                unsigned c_popcount,
                const FixedBitSet<size_> & c,
                std::vector<int> & position,
                Queue<QueueItem<size_> > * const,
                MaxCliqueResult & local_result
                ) -> void
        {
            if (best_anywhere.update(c_popcount)) {
                local_result.size = c_popcount;
                local_result.members.clear();
                for (int i = 0 ; i < graph.size() ; ++i)
                    if (c.test(i))
                        local_result.members.insert(order[i]);
                print_incumbent(params, local_result.size, position);
            }
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return best_anywhere.get();
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
            return algorithm.run();
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

    template <template <CCOPermutations, unsigned> class WhichCCO_, CCOPermutations perm_>
    struct ApplyPerm
    {
        template <unsigned size_> using Type = WhichCCO_<perm_, size_>;
    };
}

template <CCOPermutations perm_>
auto parasols::cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPerm<CCO, perm_>::template Type, MaxCliqueResult>(AllGraphSizes(), graph, params);
}

template <CCOPermutations perm_>
auto parasols::tcco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPerm<TCCO, perm_>::template Type, MaxCliqueResult>(AllGraphSizes(), graph, params);
}

template auto parasols::cco_max_clique<CCOPermutations::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::tcco_max_clique<CCOPermutations::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::tcco_max_clique<CCOPermutations::Defer1>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::tcco_max_clique<CCOPermutations::Sort>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

