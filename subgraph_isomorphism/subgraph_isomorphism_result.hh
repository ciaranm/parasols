/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_SUBGRAPH_ISOMORPHISM_RESULT_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_SUBGRAPH_ISOMORPHISM_RESULT_HH 1

#include <thread>
#include <map>
#include <list>
#include <chrono>
#include <atomic>

namespace parasols
{
    struct SubgraphIsomorphismResult
    {
        /// The isomorphism, empty if none found.
        std::map<int, int> isomorphism;

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
