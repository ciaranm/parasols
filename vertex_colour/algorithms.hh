/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_VERTEX_COLOUR_ALGORITHMS_HH
#define PARASOLS_GUARD_VERTEX_COLOUR_ALGORITHMS_HH 1

#include <vertex_colour/naive_vertex_colour.hh>
#include <vertex_colour/fc_vertex_colour.hh>

#include <utility>

namespace parasols
{
    auto algorithms = {
        std::make_pair( std::string{ "naive" },      naive_vertex_colour ),
        std::make_pair( std::string{ "fc" },         fc_vertex_colour )
    };
}

#endif
