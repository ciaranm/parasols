/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_LABELLED_CLIQUE_MAX_LABELLED_CLIQUE_RESULT_HH
#define PARASOLS_GUARD_MAX_LABELLED_CLIQUE_MAX_LABELLED_CLIQUE_RESULT_HH 1

#include <set>
#include <list>
#include <chrono>

namespace parasols
{
    /**
     * The result of a max labelled clique algorithm.
     *
     * There are various extras in here, some of which don't make sense for all
     * algorithms, allowing for a detailed analysis of behaviour.
     */
    struct MaxLabelledCliqueResult
    {
        /// Size of the best labelled clique found.
        unsigned size = 0;

        /// Cost of the best labelled clique found.
        unsigned cost = 0;

        /// Members of the best labelled clique found.
        std::set<int> members = { };

        /// Total number of nodes processed.
        unsigned long long nodes = 0;

        /**
         * Runtimes. The first entry in the list is the total runtime.
         * Additional values are for each worker thread.
         */
        std::list<std::chrono::milliseconds> times;
    };
}

#endif
