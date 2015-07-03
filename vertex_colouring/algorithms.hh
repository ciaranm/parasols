/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_VERTEX_COLOURING_ALGORITHMS_HH
#define PARASOLS_GUARD_VERTEX_COLOURING_ALGORITHMS_HH 1

#include <vertex_colouring/naive_vertex_colouring.hh>

#include <utility>

namespace parasols
{
    auto vertex_colouring_algorithms = {
        std::make_pair( std::string{ "naive" },            naive_vertex_colouring ),
    };
}

#endif
