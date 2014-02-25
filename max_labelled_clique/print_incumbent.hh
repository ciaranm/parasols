/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_LABELLED_CLIQUE_PRINT_INCUMBENT_HH
#define PARASOLS_GUARD_MAX_LABELLED_CLIQUE_PRINT_INCUMBENT_HH 1

#include <graph/graph.hh>
#include <max_labelled_clique/max_labelled_clique_params.hh>
#include <vector>
#include <set>

namespace parasols
{
    /**
     * Do some output, if params.print_incumbents is true.
     *
     * This version supports positions.
     */
    auto print_incumbent(
            const MaxLabelledCliqueParams & params,
            unsigned size,
            unsigned cost,
            const std::vector<int> & positions) -> void;

    /**
     * Do some generic string output for a position, if params.print_incumbents is true.
     */
    auto print_position(const MaxLabelledCliqueParams & params, const std::string &,
            const std::vector<int> & positions) -> void;
}

#endif
