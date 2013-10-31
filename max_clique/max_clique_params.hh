/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_PARAMS_HH
#define PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_PARAMS_HH 1

#include <list>
#include <limits>
#include <chrono>
#include <atomic>
#include <vector>

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

        /// Indicates that the graph has already been raised to this power, for
        /// s-clique (handled by the runner).
        unsigned power = 1;

        /// Number of threads to use, where appropriate.
        unsigned n_threads = 1;

        /// Splitting distance, where appropriate.
        unsigned split_depth = 1;

        /// Min size to donate, where appropriate.
        unsigned min_donate_size = 1;

        /// Override initial order and bounds, if not empty.
        std::vector<std::pair<unsigned, unsigned> > initial_order_and_bounds;

        /// If true, print every time we find a better incumbent.
        bool print_incumbents = false;

        /// If true, enable work donation.
        bool work_donation = false;

        /// If true, donate when the queue is empty, even if nothing is idle.
        bool donate_when_empty = true;

        /// Delay between choosing to donate.
        unsigned donation_wait = 0;

        /// If this is set to true, we should abort due to a time limit.
        std::atomic<bool> abort;

        /// The start time of the algorithm.
        std::chrono::time_point<std::chrono::steady_clock> start_time;
    };

    /**
     * Initial vertex ordering to use.
     */
    enum class MaxCliqueOrder
    {
        Degree, /// Prosser's 1
        Manual  /// Manual
    };
}

#endif
