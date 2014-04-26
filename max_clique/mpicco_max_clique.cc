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

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

namespace
{
    namespace Tag
    {
        enum Tag
        {
            ClientMessage,
            CurrentGlobalIncumbent,
            SubproblemForYou,
            MyIncumbent,
            MyResultSize,
            MyResultMembers,
            MyResultNodes
        };
    }

    namespace ClientMessage
    {
        enum ClientMessage
        {
            GiveMeWork,
            UpdateIncumbent
        };
    }

    template <CCOPermutations perm_, CCOInference inference_, unsigned size_, typename VertexType_>
    struct MPICCO : CCOBase<perm_, inference_, CCOMerge::None, size_, VertexType_, MPICCO<perm_, inference_, size_, VertexType_> >
    {
        using MyCCOBase = CCOBase<perm_, inference_, CCOMerge::None, size_, VertexType_, MPICCO<perm_, inference_, size_, VertexType_> >;

        using MyCCOBase::graph;
        using MyCCOBase::params;
        using MyCCOBase::expand;
        using MyCCOBase::order;
        using MyCCOBase::colour_class_order;

        mpi::environment & env;
        mpi::communicator & world;
        MaxCliqueResult result;

        unsigned last_update_nodes = 0;

        MPICCO(const Graph & g, const MaxCliqueParams & p, mpi::environment & e, mpi::communicator & w) :
            MyCCOBase(g, p),
            env(e),
            world(w)
        {
        }

        auto run() -> MaxCliqueResult
        {
            result.size = params.initial_bound;
            last_update_nodes = 0;

            if (0 == world.rank())
                return run_master();
            else
                return run_slave();
        }

        auto slave_update_incumbent() -> void
        {
            int message = ClientMessage::UpdateIncumbent;
            world.send(0, Tag::ClientMessage, message);
            unsigned global_incumbent = result.size;
            world.send(0, Tag::MyIncumbent, global_incumbent);
            world.recv(0, Tag::CurrentGlobalIncumbent, global_incumbent);
            result.size = std::max(result.size, global_incumbent);
        }

        auto run_slave() -> MaxCliqueResult
        {
            // initial colouring, just once
            FixedBitSet<size_> initial_p;
            initial_p.resize(graph.size());
            initial_p.set_all();
            std::array<VertexType_, size_ * bits_per_word> initial_p_order;
            std::array<VertexType_, size_ * bits_per_word> initial_colours;
            colour_class_order(SelectColourClassOrderOverload<perm_>(), initial_p, initial_p_order, initial_colours);

            while (true)
            {
                /* ask for something to do */
                int message = ClientMessage::GiveMeWork;
                world.send(0, Tag::ClientMessage, message);

                /* get an incumbent */
                unsigned global_incumbent;
                world.recv(0, Tag::CurrentGlobalIncumbent, global_incumbent);
                result.size = std::max(result.size, global_incumbent);

                /* get a subproblem */
                std::vector<int> subproblem;
                world.recv(0, Tag::SubproblemForYou, subproblem);

                if (subproblem.empty())
                    break;

                std::vector<unsigned> c;
                c.reserve(graph.size());

                FixedBitSet<size_> p; // potential additions
                p.resize(graph.size());
                p.set_all();

                std::vector<int> positions;
                positions.reserve(graph.size());
                positions.push_back(0);

                // go!
                expand(c, p, initial_p_order, initial_colours, positions, subproblem);

                /* transmit incumbent */
                slave_update_incumbent();
            }

            /* send result */
            world.send(0, Tag::MyResultSize, result.size);
            std::vector<int> result_members{ result.members.begin(), result.members.end() };
            world.send(0, Tag::MyResultMembers, result_members);
            world.send(0, Tag::MyResultNodes, result.nodes);

            return result;
        }

        auto run_master() -> MaxCliqueResult
        {
            auto start_time = steady_clock::now(); // local start time
            std::vector<bool> finished(world.size(), false);
            std::vector<decltype(start_time)> last_heard_from(world.size(), start_time);

            int s1 = 0, s2 = 0, n_finishes_sent = 0;
            while (n_finishes_sent < world.size() - 1) {
                /* request from anyone */
                int message;
                auto status = world.recv(mpi::any_source, Tag::ClientMessage, message);

                last_heard_from.at(status.source()) = steady_clock::now();

                if (ClientMessage::GiveMeWork == message) {
                    /* someone wants work */

                    if (s1 < graph.size()) {
                        /* send subproblem */
                        std::cerr << "-- " << duration_cast<milliseconds>(steady_clock::now() - start_time).count()
                            << " [" << result.size << "] sending subproblem " << s2 << " " << s1 << " to " << status.source() << std::endl;
                        std::vector<int> subproblem_vector = { s2, s1 };
                        world.send(status.source(), Tag::CurrentGlobalIncumbent, result.size);
                        world.send(status.source(), Tag::SubproblemForYou, subproblem_vector);

                        /* advance */
                        if (++s2 == graph.size()) {
                            s2 = 0;
                            ++s1;
                        }
                    }
                    else {
                        /* send finish */
                        std::cerr << "-- " << duration_cast<milliseconds>(steady_clock::now() - start_time).count()
                            << " [" << result.size << "] sending finish to " << status.source() << ", "
                            << world.size() - n_finishes_sent - 2 << " left" << std::endl;
                        std::vector<int> subproblem_vector;
                        world.send(status.source(), Tag::CurrentGlobalIncumbent, result.size);
                        world.send(status.source(), Tag::SubproblemForYou, subproblem_vector);
                        ++n_finishes_sent;
                        finished[status.source()] = true;

                        auto overall_time = duration_cast<milliseconds>(steady_clock::now() - start_time);
                        result.times.push_back(overall_time);

                        /* read result */
                        MaxCliqueResult sub_result;
                        std::vector<int> sub_result_members;
                        world.recv(status.source(), Tag::MyResultSize, sub_result.size);
                        world.recv(status.source(), Tag::MyResultMembers, sub_result_members);
                        sub_result.members = std::set<int>{ sub_result_members.begin(), sub_result_members.end() };
                        world.recv(status.source(), Tag::MyResultNodes, sub_result.nodes);
                        result.merge(sub_result);
                    }
                }
                else if (ClientMessage::UpdateIncumbent == message) {
                    /* just an incumbent update */
                    unsigned local_incumbent;
                    world.recv(status.source(), Tag::MyIncumbent, local_incumbent);
                    if (local_incumbent > result.size)
                        std::cerr << "-- " << duration_cast<milliseconds>(steady_clock::now() - start_time).count()
                            << " [" << result.size << "] got incumbent " << local_incumbent << " from " << status.source() << std::endl;
                    result.size = std::max(result.size, local_incumbent);
                    world.send(status.source(), Tag::CurrentGlobalIncumbent, result.size);
                }
                else {
                    std::cerr << "eek! protocol error" << std::endl;
                }

                auto current_time = steady_clock::now(); // local start time
                for (int h = 1, h_end = world.size() ; h != h_end ; ++h)
                    if (! finished[h])
                        if (duration_cast<milliseconds>(current_time - last_heard_from[h]) > milliseconds(120000)) {
                            std::cerr << "!!! haven't heard from " << h << std::endl;
                            last_heard_from[h] = current_time;
                        }
            }

            std::cerr << "-- " << duration_cast<milliseconds>(steady_clock::now() - start_time).count()
                << " all finishes sent" << std::endl;

            return result;
        }

        auto increment_nodes(std::vector<int> &) -> void
        {
            ++result.nodes;

            if (result.nodes >= 1000000 + last_update_nodes) {
                last_update_nodes = result.nodes;
                slave_update_incumbent();
            }
        }

        auto recurse(
                std::vector<unsigned> & c,                       // current candidate clique
                FixedBitSet<size_> & p,
                const std::array<VertexType_, size_ * bits_per_word> & p_order,
                const std::array<VertexType_, size_ * bits_per_word> & colours,
                std::vector<int> & position,
                std::vector<int> & subproblem
                ) -> bool
        {
            expand(c, p, p_order, colours, position, subproblem);
            return true;
        }

        auto potential_new_best(
                const std::vector<unsigned> & c,
                const std::vector<int> &,
                std::vector<int> &) -> void
        {
            // potential new best
            if (c.size() > result.size) {
                result.size = c.size();

                result.members.clear();
                for (auto & v : c)
                    result.members.insert(order[v]);

                slave_update_incumbent();
            }
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return result.size;
        }

        auto get_skip(unsigned c_popcount, std::vector<int> & subproblem, int & skip, bool & keep_going) -> void
        {
            if (c_popcount < subproblem.size()) {
                skip = subproblem.at(c_popcount);
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

