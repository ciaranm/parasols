/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_COMMON_SUBGRAPH_MAX_COMMON_SUBGRAPH_RESULT_HH
#define PARASOLS_GUARD_MAX_COMMON_SUBGRAPH_MAX_COMMON_SUBGRAPH_RESULT_HH 1

#include <map>
#include <list>
#include <chrono>

namespace parasols
{
    /**
     * The result of a max common subgraph algorithm.
     *
     * There are various extras in here, some of which don't make sense for all
     * algorithms, allowing for a detailed analysis of behaviour.
     */
    struct MaxCommonSubgraphResult
    {
        /// Size of the best common subgraph found.
        unsigned size = 0;

        /// Isomorphism.
        std::map<int, int> isomorphism = { };

        /// Total number of nodes processed.
        unsigned long long nodes = 0;

        /// The assignment bounds.
        std::pair<unsigned, unsigned> assignment_bounds;

        /// The initial colour bound.
        unsigned initial_colour_bound;

        /**
         * Runtimes. The first entry in the list is the total runtime.
         * Additional values are for each worker thread.
         */
        std::list<std::chrono::milliseconds> times;
    };
}

#endif
