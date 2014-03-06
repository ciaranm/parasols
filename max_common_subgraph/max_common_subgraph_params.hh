/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_COMMON_SUBGRAPH_MAX_COMMON_SUBGRAPH_PARAMS_HH
#define PARASOLS_GUARD_MAX_COMMON_SUBGRAPH_MAX_COMMON_SUBGRAPH_PARAMS_HH 1

#include <graph/graph.hh>
#include <max_clique/max_clique_params.hh>
#include <max_clique/max_clique_result.hh>

#include <list>
#include <limits>
#include <chrono>
#include <atomic>
#include <vector>
#include <functional>

namespace parasols
{
    /**
     * Max clique algorithm, for max common subgraphs.
     */
    using MaxCliqueAlgorithm = std::function<MaxCliqueResult (const Graph &, const MaxCliqueParams &)>;

    /**
     * Parameters for a max common subgraph algorithm.
     *
     * Not all values make sense for all algorithms. Most of these are to allow
     * us to get more details about the search process or evaluate different
     * implementation choices, rather than key functionality.
     */
    struct MaxCommonSubgraphParams
    {
        /// Override the initial size of the incumbent.
        unsigned initial_bound = 0;

        /// Exit immediately after finding a common subgraph of this size.
        unsigned stop_after_finding = std::numeric_limits<unsigned>::max();

        /// Number of threads to use, where appropriate.
        unsigned n_threads = 1;

        /// If true, print every time we find a better incumbent.
        bool print_incumbents = false;

        /// If this is set to true, we should abort due to a time limit.
        std::atomic<bool> abort;

        /// The start time of the algorithm.
        std::chrono::time_point<std::chrono::steady_clock> start_time;

        /// The max clique function to use, if we use one.
        MaxCliqueAlgorithm max_clique_algorithm;

        /// Order function for max_clique_algorithm.
        MaxCliqueOrderFunction order_function;
    };
}

#endif
