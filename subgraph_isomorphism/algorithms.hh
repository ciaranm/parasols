/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH 1

#include <subgraph_isomorphism/naive_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/gb_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/tgb_subgraph_isomorphism.hh>

#include <utility>

namespace parasols
{
    auto subgraph_isomorphism_algorithms = {
        std::make_pair( std::string{ "naive" },            naive_subgraph_isomorphism ),

        std::make_pair( std::string{ "gb" },               gb_subgraph_isomorphism ),
        std::make_pair( std::string{ "gbbj" },             gbbj_subgraph_isomorphism ),
        std::make_pair( std::string{ "gbbjnosup" },        gbbj_nosup_subgraph_isomorphism ),
        std::make_pair( std::string{ "gbbjnocad" },        gbbj_nocad_subgraph_isomorphism ),
        std::make_pair( std::string{ "gbbjnocompose" },    gbbj_nocompose_subgraph_isomorphism ),
        std::make_pair( std::string{ "gbbjfad" },          gbbj_fad_subgraph_isomorphism ),

        std::make_pair( std::string{ "dgbbj" },            dgbbj_subgraph_isomorphism ),

        std::make_pair( std::string{ "ttgbbj" },           ttgbbj_subgraph_isomorphism ),
        std::make_pair( std::string{ "ttgbbjnocompose" },  ttgbbjnocompose_subgraph_isomorphism ),
    };
}

#endif

