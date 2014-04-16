/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_MPICCO_MAX_CLIQUE_HH
#define PARASOLS_GUARD_MAX_CLIQUE_MPICCO_MAX_CLIQUE_HH 1

#include <graph/graph.hh>

#include <cco/cco.hh>

#include <max_clique/cco_base.hh>
#include <max_clique/max_clique_params.hh>
#include <max_clique/max_clique_result.hh>

#include <boost/mpi.hpp>

namespace parasols
{
    namespace detail
    {
        namespace mpi = boost::mpi;

        /**
         * Super duper max clique algorithm, with MPI.
         */
        template <CCOPermutations, CCOInference>
        auto mpicco_max_clique(
                mpi::environment &,
                mpi::communicator &,
                const Graph & graph,
                const MaxCliqueParams & params) -> MaxCliqueResult;
    }

    using detail::mpicco_max_clique;
}

#endif
