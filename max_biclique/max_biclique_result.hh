/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_BICLIQUE_MAX_BICLIQUE_RESULT_HH
#define PARASOLS_GUARD_MAX_BICLIQUE_MAX_BICLIQUE_RESULT_HH 1

#include <set>
#include <list>
#include <chrono>

namespace parasols
{
    struct MaxBicliqueResult
    {
        /// Size of the best biclique found.
        unsigned size = 0;

        /// Members of the best biclique found.
        std::set<int> members_a = { }, members_b = { };

        /// Total number of nodes processed.
        unsigned long long nodes = 0;

        /**
         * Runtimes. The first entry in the list is the total runtime.
         * Additional values are for each worker thread.
         */
        std::list<std::chrono::milliseconds> times;

        /**
         * Merge two results together (add nodes, etc). Used by threads.
         */
        auto merge(MaxBicliqueResult &&) -> void;
    };

}

#endif
