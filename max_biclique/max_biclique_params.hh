/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_BICLIQUE_MAX_BICLIQUE_PARAMS_HH
#define PARASOLS_GUARD_MAX_BICLIQUE_MAX_BICLIQUE_PARAMS_HH 1

#include <graph/graph.hh>

#include <limits>
#include <chrono>
#include <atomic>
#include <vector>
#include <functional>

namespace parasols
{
    /**
     * Initial vertex ordering to use.
     */
    using MaxBicliqueOrderFunction = std::function<void (const Graph &, std::vector<int> &)>;

    struct MaxBicliqueParams
    {
        /// Override the initial size of the incumbent.
        unsigned initial_bound = 0;

        /// Exit immediately after finding a clique of this size.
        unsigned stop_after_finding = std::numeric_limits<unsigned>::max();

        /// Number of threads to use, where appropriate.
        unsigned n_threads = 1;

        /// If true, print every time we find a better incumbent.
        bool print_incumbents = false;

        /// If this is set to true, we should abort due to a time limit.
        std::atomic<bool> * abort;

        /// The start time of the algorithm.
        std::chrono::time_point<std::chrono::steady_clock> start_time;

        /// Initial vertex ordering.
        MaxBicliqueOrderFunction order_function;
    };

    enum class BicliqueSymmetryRemoval
    {
        None,
        Remove
    };
}

#endif
