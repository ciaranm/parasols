/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/graph.hh>
#include <boost/mpi/communicator.hpp>
#include <vector>

#ifndef PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_MPI_HH
#define PARASOLS_GUARD_MAX_CLIQUE_MAX_CLIQUE_MPI_HH 1

namespace parasols
{
    struct MPIMaxCliqueResult
    {
        /// Size of the best clique found.
        unsigned size = 0;

        /// Members of the best clique found.
        std::vector<int> members = { };

        /// Total number of nodes processed.
        unsigned long long nodes = 0;
    };

    /**
     * Graph is not const because MPI.
     */
    auto mpi_max_clique_master(boost::mpi::communicator & world, Graph & graph) -> MPIMaxCliqueResult;

    auto mpi_max_clique_worker(boost::mpi::communicator & world) -> void;
}

#endif
