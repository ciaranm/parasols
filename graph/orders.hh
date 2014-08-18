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

        using OrderFunction = std::function<void (const Graph &, std::vector<int> &)>;

        auto orders = {
            std::make_pair( std::string{ "deg" },      OrderFunction( std::bind(degree_sort, _1, _2, false) ) ),
            std::make_pair( std::string{ "revdeg" },   OrderFunction( std::bind(degree_sort, _1, _2, true) ) ),
            std::make_pair( std::string{ "ex" },       OrderFunction( std::bind(exdegree_sort, _1, _2, false) ) ),
            std::make_pair( std::string{ "revex" },    OrderFunction( std::bind(exdegree_sort, _1, _2, true) ) ),
            std::make_pair( std::string{ "dynex" },    OrderFunction( std::bind(dynexdegree_sort, _1, _2, false) ) ),
            std::make_pair( std::string{ "revdynex" }, OrderFunction( std::bind(dynexdegree_sort, _1, _2, true) ) ),
            std::make_pair( std::string{ "mw" },       OrderFunction( std::bind(min_width_sort, _1, _2, false) ) ),
            std::make_pair( std::string{ "mwsi" },     OrderFunction( std::bind(mwsi_sort, _1, _2) ) ),
            std::make_pair( std::string{ "mwssi" },    OrderFunction( std::bind(mwssi_sort, _1, _2) ) ),
            std::make_pair( std::string{ "revmw" },    OrderFunction( std::bind(min_width_sort, _1, _2, true) ) ),
            std::make_pair( std::string{ "none" },     OrderFunction( std::bind(none_sort, _1, _2, false) ) ),
            std::make_pair( std::string{ "rev" },      OrderFunction( std::bind(none_sort, _1, _2, true) ) )
        };
    }

    using detail::orders;
}

#endif
