/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_ALGORITHMS_HH 1

#include <subgraph_isomorphism/naive_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/vb_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/tvb_subgraph_isomorphism.hh>

#include <utility>

namespace parasols
{
    auto subgraph_isomorphism_algorithms = {
        std::make_pair( std::string{ "naive" },               naive_subgraph_isomorphism ),

        std::make_pair( std::string{ "vbdpd" },               vb_dpd_subgraph_isomorphism ),
        std::make_pair( std::string{ "vbbjdpd" },             vbbj_dpd_subgraph_isomorphism ),
        std::make_pair( std::string{ "vbbjdpdnosup" },        vbbj_dpd_nosup_subgraph_isomorphism ),
        std::make_pair( std::string{ "vbbjdpdnocad" },        vbbj_dpd_nocad_subgraph_isomorphism ),
        std::make_pair( std::string{ "vbbjdpdfad" },          vbbj_dpd_fad_subgraph_isomorphism ),

        std::make_pair( std::string{ "tvbbjdpd" },            tvbbj_dpd_subgraph_isomorphism ),
    };
}

#endif

