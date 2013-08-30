/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_ROOMMATES_ROOMMATES_RESULT_HH
#define PARASOLS_GUARD_ROOMMATES_ROOMMATES_RESULT_HH 1

#include <set>
#include <utility>
#include <list>
#include <chrono>

namespace parasols
{
    /**
     * The result of a max clique algorithm.
     *
     * There are various extras in here, some of which don't make sense for all
     * algorithms, allowing for a detailed analysis of behaviour.
     */
    struct RoommatesResult
    {
        /// Did we succeed?
        bool success = false;

        /// Pairings, if success is true.
        std::set<std::pair<int, int> > members = { };

        /**
         * Runtimes. The first entry in the list is the total runtime.
         * Additional values are for each worker thread.
         */
        std::list<std::chrono::milliseconds> times;
    };
}

#endif
