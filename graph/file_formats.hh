/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_FILE_FORMATS_HH
#define PARASOLS_GUARD_GRAPH_FILE_FORMATS_HH 1

#include <graph/pairs.hh>
#include <graph/net.hh>
#include <graph/dimacs.hh>
#include <graph/metis.hh>
#include <graph/mivia.hh>
#include <graph/adj.hh>
#include <graph/lad.hh>

#include <utility>
#include <functional>

namespace parasols
{
    namespace detail
    {
        using GraphFileFormatFunction = std::function<Graph (const std::string &, const GraphOptions &)>;

        using namespace std::placeholders;

        auto graph_file_formats = {
            std::make_pair( std::string{ "dimacs" },  GraphFileFormatFunction{ std::bind(read_dimacs, _1, _2) } ),
            std::make_pair( std::string{ "pairs0" },  GraphFileFormatFunction{ std::bind(read_pairs, _1, false, _2) } ),
            std::make_pair( std::string{ "pairs1" },  GraphFileFormatFunction{ std::bind(read_pairs, _1, true, _2) } ),
            std::make_pair( std::string{ "net" },     GraphFileFormatFunction{ std::bind(read_net, _1, _2) } ),
            std::make_pair( std::string{ "metis" },   GraphFileFormatFunction{ std::bind(read_metis, _1, _2) } ),
            std::make_pair( std::string{ "mivia" },   GraphFileFormatFunction{ std::bind(read_mivia, _1, _2) } ),
            std::make_pair( std::string{ "adj" },     GraphFileFormatFunction{ std::bind(read_adj, _1, _2) } ),
            std::make_pair( std::string{ "lad" },     GraphFileFormatFunction{ std::bind(read_lad, _1, _2) } )
        };
    }

    using detail::graph_file_formats;
}

#endif
