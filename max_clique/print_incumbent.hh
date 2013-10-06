/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_PRINT_INCUMBENT_HH
#define PARASOLS_GUARD_MAX_CLIQUE_PRINT_INCUMBENT_HH 1

#include <max_clique/max_clique_params.hh>
#include <vector>

namespace parasols
{
    /**
     * Do some output, if params.print_incumbents is true.
     */
    auto print_incumbent(const MaxCliqueParams & params, unsigned size) -> void;

    /**
     * Do some output, if params.print_incumbents is true.
     */
    auto print_incumbent(const MaxCliqueParams & params, unsigned size,
            const std::vector<int> & positions) -> void;
}

#endif
