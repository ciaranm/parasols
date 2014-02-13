/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_ALGORITHMS_HH
#define PARASOLS_GUARD_MAX_CLIQUE_ALGORITHMS_HH 1

#include <max_clique/naive_max_clique.hh>
#include <max_clique/bmcsa_max_clique.hh>
#include <max_clique/tbmcsa_max_clique.hh>
#include <max_clique/dbmcsa_max_clique.hh>
#include <max_clique/cco_max_clique.hh>
#include <max_clique/tcco_max_clique.hh>

#include <utility>
#include <string>

namespace parasols
{
    auto max_clique_algorithms = {
        std::make_pair( std::string{ "naive" },     naive_max_clique),

        std::make_pair( std::string{ "bmcsa" },     bmcsa_max_clique),

        std::make_pair( std::string{ "tbmcsa" },    tbmcsa_max_clique ),

        std::make_pair( std::string{ "dbmcsa" },    dbmcsa_max_clique ),

        std::make_pair( std::string{ "ccon" },      cco_max_clique<CCOPermutations::None>),
        std::make_pair( std::string{ "ccod" },      cco_max_clique<CCOPermutations::Defer1>),
        std::make_pair( std::string{ "ccos" },      cco_max_clique<CCOPermutations::Sort>),

        std::make_pair( std::string{ "tccon" },     tcco_max_clique<CCOPermutations::None>),
        std::make_pair( std::string{ "tccod" },     tcco_max_clique<CCOPermutations::Defer1>),
        std::make_pair( std::string{ "tccos" },     tcco_max_clique<CCOPermutations::Sort>)
    };
}

#endif
