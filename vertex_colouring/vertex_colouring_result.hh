/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_VERTEX_COLOURING_VERTEX_COLOURING_RESULT_HH
#define PARASOLS_GUARD_VERTEX_COLOURING_VERTEX_COLOURING_RESULT_HH 1

#include <thread>
#include <map>
#include <list>
#include <chrono>
#include <atomic>

namespace parasols
{
    struct VertexColouringResult
    {
        /// Number of colours used
        int n_colours = 0;

        /// The colouring
        std::map<int, int> colouring;

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
