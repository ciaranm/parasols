/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_LABELLED_CLIQUE_ALGORITHMS_HH
#define PARASOLS_GUARD_MAX_LABELLED_CLIQUE_ALGORITHMS_HH 1

#include <max_labelled_clique/lcco_max_labelled_clique.hh>

#include <utility>
#include <string>

namespace parasols
{
    auto max_labelled_clique_algorithms = {
        std::make_pair( std::string{ "lcco" },     lcco_max_labelled_clique)
    };
}

#endif
