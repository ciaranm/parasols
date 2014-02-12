/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/mpi_max_clique.hh>
#include <max_clique/colourise.hh>

#include <boost/mpi.hpp>

using namespace parasols;
namespace mpi = boost::mpi;

namespace
{
    auto expand(
            const Graph & graph,
            Buckets & buckets,                       // pre-allocated
            std::vector<int> & c,                    // current candidate clique
            std::vector<int> & o,                    // potential additions, in order
            MPIMaxCliqueResult & result,
            int subproblem) -> void
    {
        ++result.nodes;

        // get our coloured vertices
        std::vector<int> p;
        p.reserve(o.size());
        auto colours = colourise(graph, buckets, p, o);

        // for each v in p... (v comes later)
        int at = 0;
        for (int n = p.size() - 1 ; n >= 0 ; --n, ++at) {

            // bound, timeout or early exit?
            if (c.size() + colours[n] <= result.size)
                return;

            auto v = p[n];

            if (-1 == subproblem || at == subproblem) {
                // consider taking v
                c.push_back(v);

                // filter o to contain vertices adjacent to v
                std::vector<int> new_o;
                new_o.reserve(o.size());
                std::copy_if(o.begin(), o.end(), std::back_inserter(new_o), [&] (int w) { return graph.adjacent(v, w); });

                if (new_o.empty()) {
                    // potential new best
                    if (c.size() > result.size) {
                        result.size = c.size();
                        result.members = std::vector<int>{ c.begin(), c.end() };
                        std::sort(result.members.begin(), result.members.end());
                    }
                }
                else
                    expand(graph, buckets, c, new_o, result, -1);

                c.pop_back();
            }

            // now consider not taking v
            p.pop_back();
            o.erase(std::remove(o.begin(), o.end(), v));
        }
    }

    auto max_clique(
            mpi::communicator & world,
            const Graph & graph) -> MPIMaxCliqueResult
    {
        MPIMaxCliqueResult result;

        auto buckets = make_buckets(graph.size());

        while (true) {
            // get a job
            int subproblem = 0;
            world.recv(0, 2, subproblem);
            if (-1 == subproblem)
                break;

            world.recv(0, 4, result.size);

            std::vector<int> o(graph.size()); // potential additions, ordered
            std::iota(o.begin(), o.end(), 0);
            params.order_function(graph, o);

            std::vector<int> c; // current candidate clique
            c.reserve(graph.size());

            // go!
            expand(graph, buckets, c, o, result, subproblem);

            world.send(0, 3, result.size);
        }

        return result;
    }
}

auto parasols::mpi_max_clique_master(
        mpi::communicator & world,
        Graph & graph) -> MPIMaxCliqueResult
{
    unsigned size = graph.size();
    broadcast(world, size, 0);
    broadcast(world, graph.adjaceny_matrix(), 0);

    std::vector<int> jobs((graph.size()));
    std::iota(jobs.begin(), jobs.end(), 0);
    std::reverse(jobs.begin(), jobs.end());

    std::vector<int> available((world.size() - 1)), busy;
    std::iota(available.begin(), available.end(), 1);

    unsigned best_omega = 0;
    while (true) {
        if ((! jobs.empty()) && (! available.empty())) {
            int job = jobs.back();
            jobs.pop_back();
            int send_to = available.back();
            available.pop_back();
            busy.push_back(send_to);

            std::cerr << "Sending " << job << " to " << send_to << std::endl;
            world.send(send_to, 2, job);
            world.send(send_to, 4, best_omega);
        }
        else if (! busy.empty()) {
            std::cerr << "Waiting for someone to finish" << std::endl;
            int source = world.probe(mpi::any_source, 3).source();

            unsigned omega;
            world.recv(source, 3, omega);
            best_omega = std::max(best_omega, omega);

            std::cerr << "Got " << omega << " from " << source << std::endl;
            available.push_back(source);
            busy.erase(std::find(busy.begin(), busy.end(), source));
        }
        else if (jobs.empty() && busy.empty()) {
            std::cerr << "All done" << std::endl;
            break;
        }
        else
            throw 0;
    }

    for (int i = 1 ; i < world.size() ; ++i)
        world.send(i, 2, -1);

    MPIMaxCliqueResult result;
    for (int i = 1 ; i < world.size() ; ++i) {
        MPIMaxCliqueResult local_result;
        world.recv(i, 1, local_result.size);
        world.recv(i, 1, local_result.nodes);
        world.recv(i, 1, local_result.members);

        if (result.size < local_result.size) {
            result.size = local_result.size;
            result.members = std::move(local_result.members);
        }

        result.nodes += local_result.nodes;
    }

    if (best_omega != result.size)
        throw 0;

    return result;
}

auto parasols::mpi_max_clique_worker(
        mpi::communicator & world) -> void
{
    unsigned size = 0;
    broadcast(world, size, 0);

    Graph graph(size, false);
    broadcast(world, graph.adjaceny_matrix(), 0);

    auto result = max_clique(world, graph);

    world.send(0, 1, result.size);
    world.send(0, 1, result.nodes);
    world.send(0, 1, result.members);
}

