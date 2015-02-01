/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH 1

#include <subgraph_isomorphism/naive_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/cb_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/vb_subgraph_isomorphism.hh>

#include <utility>

namespace parasols
{
    auto subgraph_isomorphism_algorithms = {
        std::make_pair( std::string{ "naive" },               naive_subgraph_isomorphism ),

        std::make_pair( std::string{ "sb" },                  sb_subgraph_isomorphism ),
        std::make_pair( std::string{ "sbbj" },                sbbj_subgraph_isomorphism ),
        std::make_pair( std::string{ "sbbjfad" },             sbbjfad_subgraph_isomorphism ),

        std::make_pair( std::string{ "vb" },                  vb_subgraph_isomorphism ),
        std::make_pair( std::string{ "vbbj" },                vbbj_subgraph_isomorphism ),
        std::make_pair( std::string{ "vbbjnosup" },           vbbjnosup_subgraph_isomorphism ),
        std::make_pair( std::string{ "vbbjnocad" },           vbbjnocad_subgraph_isomorphism ),
        std::make_pair( std::string{ "vbbjfad" },             vbbjfad_subgraph_isomorphism ),
        std::make_pair( std::string{ "vbbj4" },               vbbj4_subgraph_isomorphism ),
    };
}

#endif

