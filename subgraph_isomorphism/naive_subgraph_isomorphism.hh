/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_NAIVE_SUBGRAPH_ISOMORPHISM_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_NAIVE_SUBGRAPH_ISOMORPHISM_HH 1

#include <graph/graph.hh>

#include <thread>
#include <map>
#include <list>
#include <chrono>
#include <atomic>

namespace parasols
{
    struct SubgraphIsomorphismParams
    {
        /// If this is set to true, we should abort due to a time limit.
        std::atomic<bool> * abort;

        /// The start time of the algorithm.
        std::chrono::time_point<std::chrono::steady_clock> start_time;

        /// Number of threads to use, where appropriate.
        unsigned n_threads = 1;

        /// Look for an induced (proper) isomorphism?
        bool induced = true;
    };

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

    auto naive_subgraph_isomorphism(const std::pair<Graph, Graph> &, const SubgraphIsomorphismParams &) -> SubgraphIsomorphismResult;
}

#endif
