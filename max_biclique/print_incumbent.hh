/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_BICLIQUE_PRINT_INCUMBENT_HH
#define PARASOLS_GUARD_MAX_BICLIQUE_PRINT_INCUMBENT_HH 1

#include <max_biclique/max_biclique_params.hh>

namespace parasols
{
    /**
     * Do some output, if params.print_incumbents is true.
     */
    auto print_incumbent(const MaxBicliqueParams & params, unsigned size) -> void;
}

#endif
