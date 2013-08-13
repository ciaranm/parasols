/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_PARAMS_HH
#define PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_PARAMS_HH 1

#include <set>
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
     * The result of a max clique algorithm.
     *
     * There are various extras in here, some of which don't make sense for all
     * algorithms, allowing for a detailed analysis of behaviour.
     */
    struct MaxCliqueResult
    {
        /// Size of the best clique found.
        unsigned size = 0;

        /// Members of the best clique found.
        std::set<int> members = { };

        /// Total number of nodes processed.
        unsigned long long nodes = 0;

        /// Number of times work donation occurred.
        unsigned donations = 0;

        /**
         * Runtimes. The first entry in the list is the total runtime.
         * Additional values are for each worker thread.
         */
        std::list<std::chrono::milliseconds> times;

        /**
         * Number of 'top' nodes we've successfully processed. Potentially
         * useful for guessing how far we've progressed if a timeout occurs.
         */
        unsigned long long top_nodes_done = 0;

        /**
         * Merge two results together (add nodes, etc). Used by threads.
         */
        auto merge(const MaxCliqueResult &) -> void;
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
