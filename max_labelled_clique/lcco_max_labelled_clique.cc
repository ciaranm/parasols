/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_labelled_clique/lcco_max_labelled_clique.hh>
#include <max_labelled_clique/print_incumbent.hh>
#include <max_labelled_clique/lcco_base.hh>

#include <cco/cco_mixin.hh>
#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>

using namespace parasols;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

namespace
{
    template <CCOPermutations perm_, unsigned size_, typename VertexType_>
    struct LCCO :
        LCCOBase<perm_, size_, VertexType_, LCCO<perm_, size_, VertexType_> >
    {
        using LCCOBase<perm_, size_, VertexType_, LCCO<perm_, size_, VertexType_> >::LCCOBase;
        using LCCOBase<perm_, size_, VertexType_, LCCO<perm_, size_, VertexType_> >::params;
        using LCCOBase<perm_, size_, VertexType_, LCCO<perm_, size_, VertexType_> >::graph;
        using LCCOBase<perm_, size_, VertexType_, LCCO<perm_, size_, VertexType_> >::order;
        using LCCOBase<perm_, size_, VertexType_, LCCO<perm_, size_, VertexType_> >::expand;
        using LCCOBase<perm_, size_, VertexType_, LCCO<perm_, size_, VertexType_> >::colour_class_order;

        MaxLabelledCliqueResult result;

        auto run() -> MaxLabelledCliqueResult
        {
            result.size = params.initial_bound;

            for (unsigned pass = 1 ; pass <= 2 ; ++pass) {
                auto start_time = steady_clock::now(); // local start time

                std::vector<VertexType_> c;
                c.reserve(graph.size());

                FixedBitSet<size_> p; // potential additions
                p.set_up_to(graph.size());

                std::vector<int> positions;
                positions.reserve(graph.size());
                positions.push_back(0);

                LabelSet u;

                std::array<VertexType_, size_ * bits_per_word> initial_p_order;
                std::array<VertexType_, size_ * bits_per_word> initial_colours;
                colour_class_order(SelectColourClassOrderOverload<perm_>(), p, initial_p_order, initial_colours);

                // go!
                expand(pass == 2, c, p, u, initial_p_order, initial_colours, positions);

                auto overall_time = duration_cast<milliseconds>(steady_clock::now() - start_time);
                result.times.push_back(overall_time);
            }

            return result;
        }

        auto potential_new_best(
                unsigned c_popcount,
                const std::vector<VertexType_> & c,
                unsigned cost,
                std::vector<int> & position) -> void
        {
            // potential new best
            if (c_popcount > result.size || (c_popcount == result.size && cost < result.cost)) {
                result.size = c_popcount;
                result.cost = cost;

                result.members.clear();
                for (auto & v : c)
                    result.members.insert(order[v]);

                print_incumbent(params, c_popcount, cost, position);
            }
        }

        auto recurse(
                bool pass_2,
                std::vector<VertexType_> & c,
                FixedBitSet<size_> & p,
                LabelSet & u,
                const std::array<VertexType_, size_ * bits_per_word> & p_order,
                const std::array<VertexType_, size_ * bits_per_word> & colours,
                std::vector<int> & position
                ) -> bool
        {
            expand(pass_2, c, p, u, p_order, colours, position);
            return true;
        }

        auto increment_nodes() -> void
        {
            ++result.nodes;
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return result.size;
        }

        auto get_best_anywhere_cost() -> unsigned
        {
            return result.cost;
        }

        auto get_skip(unsigned, int &, bool &) -> void
        {
        }
    };
}

template <CCOPermutations perm_>
auto parasols::lcco_max_labelled_clique(const Graph & graph, const MaxLabelledCliqueParams & params) -> MaxLabelledCliqueResult
{
    return select_graph_size<ApplyPerm<LCCO, perm_>::template Type, MaxLabelledCliqueResult>(AllGraphSizes(), graph, params);
}

template auto parasols::lcco_max_labelled_clique<CCOPermutations::None>(const Graph &, const MaxLabelledCliqueParams &) -> MaxLabelledCliqueResult;
template auto parasols::lcco_max_labelled_clique<CCOPermutations::Defer1>(const Graph &, const MaxLabelledCliqueParams &) -> MaxLabelledCliqueResult;

