/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_ORDERS_HH
#define PARASOLS_GUARD_GRAPH_ORDERS_HH 1

#include <graph/degree_sort.hh>
#include <graph/min_width_sort.hh>

#include <utility>
#include <functional>

namespace parasols
{
    namespace detail
    {
        using namespace std::placeholders;

        auto orders = {
            std::make_pair( std::string{ "deg" },     std::bind(degree_sort, _1, _2, false) ),
            std::make_pair( std::string{ "ex" },      std::bind(exdegree_sort, _1, _2, false) ),
            std::make_pair( std::string{ "dynex" },   std::bind(dynexdegree_sort, _1, _2, false) ),
            std::make_pair( std::string{ "mw" },      std::bind(min_width_sort, _1, _2, false) ),
            std::make_pair( std::string{ "none" },    std::bind(none_sort, _1, _2, false) )
        };
    }

    using detail::orders;
}

#endif
