/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_FILE_FORMATS_HH
#define PARASOLS_GUARD_GRAPH_FILE_FORMATS_HH 1

#include <graph/pairs.hh>
#include <graph/net.hh>
#include <graph/dimacs.hh>

#include <utility>
#include <functional>

namespace parasols
{
    using GraphFileFormatFunction = std::function<Graph (const std::string &)>;

    auto graph_file_formats = {
        std::make_pair( std::string{ "dimacs" },  GraphFileFormatFunction{ std::bind(read_dimacs, std::placeholders::_1) } ),
        std::make_pair( std::string{ "pairs0" },  GraphFileFormatFunction{ std::bind(read_pairs, std::placeholders::_1, false) } ),
        std::make_pair( std::string{ "pairs1" },  GraphFileFormatFunction{ std::bind(read_pairs, std::placeholders::_1, true) } ),
        std::make_pair( std::string{ "net" },     GraphFileFormatFunction{ std::bind(read_net, std::placeholders::_1) } )
    };
}

#endif
