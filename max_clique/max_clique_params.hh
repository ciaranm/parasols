/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_PARAMS_HH
#define PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_PARAMS_HH 1

#include <max_clique/max_clique_result.hh>

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

        /// Number of threads to use, where appropriate.
        unsigned n_threads = 1;

        /// Splitting distance, where appropriate.
        unsigned split_depth = 1;

        /// If true, print every time we find a better incumbent.
        bool print_candidates = false;

        /// If true, enable work donation.
        bool work_donation = false;

        /// If this is set to true, we should abort due to a time limit.
        std::atomic<bool> abort;

        /// The start time of the algorithm.
        std::chrono::time_point<std::chrono::steady_clock> start_time;
    };

    /**
     * Do some output, if params.print_candidate is true.
     */
    auto print_candidate(const MaxCliqueParams & params, unsigned size) -> void;

    /**
     * Do some output, if params.print_candidate is true.
     */
    auto print_candidate(
            const MaxCliqueParams & params,
            const MaxCliqueResult & result,
            const std::vector<std::pair<int, int> > & choices_of_n) -> void;
}

#endif
