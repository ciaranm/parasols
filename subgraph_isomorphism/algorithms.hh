/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH 1

#include <subgraph_isomorphism/naive_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/b_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/mb_subgraph_isomorphism.hh>

#include <utility>

namespace parasols
{
    auto subgraph_isomorphism_algorithms = {
        std::make_pair( std::string{ "naive" },         naive_subgraph_isomorphism ),
        std::make_pair( std::string{ "blv" },           blv_subgraph_isomorphism ),
        std::make_pair( std::string{ "blvh" },          blvh_subgraph_isomorphism ),
        std::make_pair( std::string{ "blvc23" },        blvc23_subgraph_isomorphism ),
        std::make_pair( std::string{ "blvc23h" },       blvc23h_subgraph_isomorphism ),
        std::make_pair( std::string{ "blvc23i" },       blvc23i_subgraph_isomorphism ),
        std::make_pair( std::string{ "badlvc23h" },     badlvc23h_subgraph_isomorphism ),

        std::make_pair( std::string{ "mb" },            mb_subgraph_isomorphism )
    };
}

#endif

