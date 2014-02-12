/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_PARAMS_HH
#define PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_PARAMS_HH 1

#include <list>
#include <limits>
#include <chrono>
#include <atomic>
#include <vector>
#include <graph/graph.hh>

namespace parasols
{
    /**
     * Parameters for a max clique algorithm.
     *
     * Not all values make sense for all algorithms. Most of these are to allow
     * us to get more details about the search process or evaluate different
     * implementation choices, rather than key functionality.
     */
    struct MaxCliqueParams
    {
        /// Override the initial size of the incumbent.
        unsigned initial_bound = 0;

        /// Exit immediately after finding a clique of this size.
        unsigned stop_after_finding = std::numeric_limits<unsigned>::max();

        /// Enumerate solutions (only works with initial_bound).
        bool enumerate = false;

        /// Indicates that the complement has been taken, for independent set
        /// (handled by the runner).
        bool complement = false;

        /// Indicates that the graph has already been raised to this power, for
        /// s-clique (handled by the runner).
        unsigned power = 1;

        /// Number of threads to use, where appropriate.
        unsigned n_threads = 1;

        /// Splitting distance, where appropriate.
        unsigned split_depth = 1;

        /// If true, print every time we find a better incumbent.
        bool print_incumbents = false;

        /// If true, and print_incumbents is also true, show whether or not the
        /// new incumbent is a club (slow).
        bool check_clubs = false;

        /// If true, enable work donation.
        bool work_donation = false;

        /// If this is set to true, we should abort due to a time limit.
        std::atomic<bool> abort;

        /// The start time of the algorithm.
        std::chrono::time_point<std::chrono::steady_clock> start_time;

        /// The unmangled graph, for club verification.
        const Graph * original_graph = nullptr;
    };

    /**
     * Initial vertex ordering to use.
     */
    enum class MaxCliqueOrder
    {
        Degree,     /// Prosser's style 1
        MinWidth,   /// Prosser's style 2
        ExDegree,   /// Prosser's style 3
        DynExDegree /// Tomita's MCR ordering (?)
    };

    /**
     * Used by CCO variants to control permutations.
     */
    enum class CCOPermutations
    {
        None,
        Defer1,
        Sort
    };
}

#endif
