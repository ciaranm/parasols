/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH 1

#include <subgraph_isomorphism/naive_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/cb_subgraph_isomorphism.hh>

#include <utility>

namespace parasols
{
    auto subgraph_isomorphism_algorithms = {
        std::make_pair( std::string{ "naive" },               naive_subgraph_isomorphism ),

        std::make_pair( std::string{ "cbjdp" },               cbjdp_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdpi" },              cbjdpi_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdpirand" },          cbjdpi_rand_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdpiranddeg" },       cbjdpi_randdeg_subgraph_isomorphism ),
    };
}

#endif

