/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/mpicco_max_clique.hh>
#include <max_clique/cco_base.hh>

#include <graph/template_voodoo.hh>

#include <boost/mpi.hpp>

#include <algorithm>
#include <thread>
#include <mutex>

using namespace parasols;
namespace mpi = boost::mpi;

namespace
{
    template <CCOPermutations perm_, CCOInference inference_, unsigned size_, typename VertexType_>
    struct MPICCO : CCOBase<perm_, inference_, size_, VertexType_, MPICCO<perm_, inference_, size_, VertexType_> >
    {
        using MyCCOBase = CCOBase<perm_, inference_, size_, VertexType_, MPICCO<perm_, inference_, size_, VertexType_> >;

        using MyCCOBase::graph;
        using MyCCOBase::params;
        using MyCCOBase::expand;
        using MyCCOBase::order;
        using MyCCOBase::colour_class_order;

        mpi::environment & env;
        mpi::communicator & world;
        MaxCliqueResult result;

        MPICCO(const Graph & g, const MaxCliqueParams & p, mpi::environment & e, mpi::communicator & w) :
            MyCCOBase(g, p),
            env(e),
            world(w)
        {
        }

        auto run() -> MaxCliqueResult
        {
            result.size = params.initial_bound;

            if (0 == world.rank())
                return run_master();
            else
                return run_slave();
        }

        auto run_slave() -> MaxCliqueResult
        {
            while (true)
            {
                /* ask for something to do */
                int message = 0;
                world.send(0, 1000, message);

                /* get a subproblem */
                world.recv(0, 1001, message);

                if (-1 == message)
                    break;

                int subproblem = message;

                std::vector<unsigned> c;
                c.reserve(graph.size());

                FixedBitSet<size_> p; // potential additions
                p.resize(graph.size());
                p.set_all();

                std::vector<int> positions;
                positions.reserve(graph.size());
                positions.push_back(0);

                // initial colouring
                std::array<VertexType_, size_ * bits_per_word> initial_p_order;
                std::array<VertexType_, size_ * bits_per_word> initial_colours;
                colour_class_order(SelectColourClassOrderOverload<perm_>(), p, initial_p_order, initial_colours);

                // go!
                expand(c, p, initial_p_order, initial_colours, positions, subproblem);
            }

            /* send result */
            world.send(0, 1002, result.size);
            std::vector<int> result_members{ result.members.begin(), result.members.end() };
            world.send(0, 1003, result_members);
            world.send(0, 1004, result.nodes);

            return result;
        }

        auto run_master() -> MaxCliqueResult
        {
            int subproblem = 0, n_finishes_sent = 0;
            while (n_finishes_sent < world.size() - 1) {
                /* request from anyone */
                int message;
                auto status = world.recv(mpi::any_source, 1000, message);

                if (subproblem < graph.size()) {
                    /* send subproblem */
                    world.send(status.source(), 1001, subproblem);
                    std::cerr << "sending subproblem " << subproblem << " to " << status.source() << std::endl;
                    ++subproblem;
                }
                else {
                    /* send finish */
                    int finish = -1;
                    std::cerr << "sending finish to " << status.source() << std::endl;
                    world.send(status.source(), 1001, finish);
                    ++n_finishes_sent;

                    /* read result */
                    MaxCliqueResult sub_result;
                    std::vector<int> sub_result_members;
                    world.recv(status.source(), 1002, sub_result.size);
                    world.recv(status.source(), 1003, sub_result_members);
                    sub_result.members = std::set<int>{ sub_result_members.begin(), sub_result_members.end() };
                    world.recv(status.source(), 1004, sub_result.nodes);
                    result.merge(sub_result);
                }
            }

            std::cerr << "all finishes sent" << std::endl;

            return result;
        }

        auto increment_nodes(int) -> void
        {
            ++result.nodes;
        }

        auto recurse(
                std::vector<unsigned> & c,                       // current candidate clique
                FixedBitSet<size_> & p,
                const std::array<VertexType_, size_ * bits_per_word> & p_order,
                const std::array<VertexType_, size_ * bits_per_word> & colours,
                std::vector<int> & position,
                int subproblem
                ) -> bool
        {
            expand(c, p, p_order, colours, position, subproblem);
            return true;
        }

        auto potential_new_best(
                const std::vector<unsigned> & c,
                const std::vector<int> &,
                int) -> void
        {
            // potential new best
            if (c.size() > result.size) {
                result.size = c.size();

                result.members.clear();
                for (auto & v : c)
                    result.members.insert(order[v]);
            }
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return result.size;
        }

        auto get_skip(unsigned c_popcount, int subproblem, int & skip, bool & keep_going) -> void
        {
            if (0 == c_popcount) {
                skip = subproblem;
                keep_going = false;
            }
        }
    };
}

template <CCOPermutations perm_, CCOInference inference_>
auto parasols::detail::mpicco_max_clique(
        mpi::environment & env,
        mpi::communicator & world,
        const Graph & graph,
        const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPermInference<MPICCO, perm_, inference_>::template Type, MaxCliqueResult>(
            AllGraphSizes(), graph, params, env, world);
}

template auto parasols::detail::mpicco_max_clique<CCOPermutations::None, CCOInference::None>(
        mpi::environment &, mpi::communicator &, const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

