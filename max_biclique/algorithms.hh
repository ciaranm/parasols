/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_BICLIQUE_ALGORITHMS_HH
#define PARASOLS_GUARD_MAX_BICLIQUE_ALGORITHMS_HH 1

#include <max_biclique/naive_max_biclique.hh>
#include <max_biclique/ccd_max_biclique.hh>
#include <max_biclique/dccd_max_biclique.hh>

#include <utility>

namespace parasols
{
    auto max_biclique_algorithms = {
        std::make_pair( std::string{ "naive" },      naive_max_biclique ),
        std::make_pair( std::string{ "ccd" },        ccd_max_biclique ),
        std::make_pair( std::string{ "dccd" },       dccd_max_biclique )
    };
}

#endif
