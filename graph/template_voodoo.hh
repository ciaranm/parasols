/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_TEMPLATE_VOODOO_HH
#define PARASOLS_GUARD_GRAPH_TEMPLATE_VOODOO_HH 1

#include <graph/graph.hh>
#include <graph/bit_graph.hh>

#include <type_traits>

namespace parasols
{
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

    template <unsigned n_>
    struct IndexSizes
    {
        using Type = typename std::conditional<n_ * bits_per_word <= 1ul << (8 * sizeof(unsigned char)), unsigned char,
              typename std::conditional<n_ * bits_per_word <= 1ul << (8 * sizeof(unsigned short)), unsigned short,
                std::false_type>::type>::type;
    };

    template <template <unsigned, typename> class Algorithm_, typename Result_, typename Graph_, unsigned... sizes_, typename... Params_>
    auto select_graph_size(const GraphSizes<sizes_...> &, const Graph_ & graph, Params_ && ... params) -> Result_
    {
        if (graph.size() < GraphSizes<sizes_...>::n * bits_per_word) {
            Algorithm_<GraphSizes<sizes_...>::n, typename IndexSizes<GraphSizes<sizes_...>::n>::Type> algorithm{
                graph, std::forward<Params_>(params)... };
            return algorithm.run();
        }
        else
            return select_graph_size<Algorithm_, Result_, Graph_>(typename GraphSizes<sizes_...>::Rest(), graph, std::forward<Params_>(params)...);
    }

    template <template <unsigned, typename> class Algorithm_, typename Result_, typename Graph_, typename... Params_>
    auto select_graph_size(const NoMoreGraphSizes &, const Graph_ &, Params_ && ...) -> Result_
    {
        throw GraphTooBig();
    }

    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 1024, "Need to update here if max_graph_size is changed.");

    using AllGraphSizes = GraphSizes<1, 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128, 256, 512, 1024>;
}

#endif
