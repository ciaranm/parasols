/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CLIQUE_GUARD_CLIQUE_TMCSA1_MAX_CLIQUE_HH
#define CLIQUE_GUARD_CLIQUE_TMCSA1_MAX_CLIQUE_HH 1

#include <clique/graph.hh>
#include <clique/max_clique_params.hh>

namespace clique
{
    /**
     * Threaded max clique algorithm.
     *
     * This is our threaded version of Tomita's MCS with non-increasing degree
     * ordering and no colour repair step, or Prosser's MCSa1.
     *
     * This variation uses a mutex for sharing the incumbent.
     */
    auto tmcsa1_mutex_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;

    /**
     * Threaded max clique algorithm.
     *
     * This is our threaded version of Tomita's MCS with non-increasing degree
     * ordering and no colour repair step, or Prosser's MCSa1.
     *
     * This variation uses a shared mutex for sharing the incumbent.
     */
    auto tmcsa1_shared_mutex_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;

    /**
     * Threaded max clique algorithm.
     *
     * This is our threaded version of Tomita's MCS with non-increasing degree
     * ordering and no colour repair step, or Prosser's MCSa1.
     *
     * This variation uses an atomic for sharing the incumbent.
     */
    auto tmcsa1_atomic_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult;
}

#endif
