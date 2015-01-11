/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH 1

#include <subgraph_isomorphism/naive_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/cb_subgraph_isomorphism.hh>

#include <utility>

namespace parasols
{
    auto subgraph_isomorphism_algorithms = {
        std::make_pair( std::string{ "naive" },         naive_subgraph_isomorphism ),

        std::make_pair( std::string{ "cb" },            cb_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbj" },           cbj_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbd" },           cbd_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjd" },          cbjd_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdnoalldiff" }, cbjdnoalldiff_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdnom3" },      cbjdnom3_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdnom23" },     cbjdnom23_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdnod3" },      cbjdnod3_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdnod23" },     cbjdnod23_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbfast" },        cbfast_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjfast" },       cbjfast_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbdfast" },       cbdfast_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdfast" },      cbjdfast_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdprobe1" },    cbjdprobe1_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdprobe2" },    cbjdprobe2_subgraph_isomorphism ),
        std::make_pair( std::string{ "cbjdprobe3" },    cbjdprobe3_subgraph_isomorphism ),
    };
}

#endif

