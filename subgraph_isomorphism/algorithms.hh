/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH 1

#include <subgraph_isomorphism/naive_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/b_subgraph_isomorphism.hh>

#include <utility>

namespace parasols
{
    auto subgraph_isomorphism_algorithms = {
        std::make_pair( std::string{ "naive" },      naive_subgraph_isomorphism ),
        std::make_pair( std::string{ "b" },          b_subgraph_isomorphism ),
        std::make_pair( std::string{ "bdds" },       bdds_subgraph_isomorphism ),
        std::make_pair( std::string{ "bnds" },       bnds_subgraph_isomorphism ),
        std::make_pair( std::string{ "bndss" },      bndss_subgraph_isomorphism ),
        std::make_pair( std::string{ "bndsdds" },    bndsdds_subgraph_isomorphism ),
        std::make_pair( std::string{ "bndsp3" },     bndsp3_subgraph_isomorphism ),
        std::make_pair( std::string{ "bndsp3s" },    bndsp3s_subgraph_isomorphism ),
        std::make_pair( std::string{ "bndsrp3s" },   bndsrp3s_subgraph_isomorphism ),
        std::make_pair( std::string{ "bndsrx2s" },   bndsrx2s_subgraph_isomorphism ),
    };
}

#endif

