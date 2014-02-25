/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_labelled_clique/lcco_max_labelled_clique.hh>
#include <max_labelled_clique/print_incumbent.hh>
#include <max_labelled_clique/lcco_base.hh>

#include <cco/cco_mixin.hh>
#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>

using namespace parasols;

namespace
{
    template <CCOPermutations perm_, unsigned size_>
    struct LCCO :
        LCCOBase<perm_, size_, LCCO<perm_, size_> >
    {
        using LCCOBase<perm_, size_, LCCO<perm_, size_> >::LCCOBase;
        using LCCOBase<perm_, size_, LCCO<perm_, size_> >::params;
        using LCCOBase<perm_, size_, LCCO<perm_, size_> >::graph;
        using LCCOBase<perm_, size_, LCCO<perm_, size_> >::order;
        using LCCOBase<perm_, size_, LCCO<perm_, size_> >::expand;

        MaxLabelledCliqueResult result;

        auto run() -> MaxLabelledCliqueResult
        {
            result.size = params.initial_bound;

            for (unsigned pass = 1 ; pass <= 2 ; ++pass) {
                auto start_time = std::chrono::steady_clock::now(); // local start time

                FixedBitSet<size_> c; // current candidate clique
                c.resize(graph.size());

                FixedBitSet<size_> p; // potential additions
                p.resize(graph.size());
                p.set_all();

                std::vector<int> positions;
                positions.reserve(graph.size());
                positions.push_back(0);

                LabelSet u;

                // go!
                expand(pass == 2, c, p, u, positions);

                auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
                result.times.push_back(overall_time);
            }

            return result;
        }

        auto potential_new_best(
                unsigned c_popcount,
                const FixedBitSet<size_> & c,
                unsigned cost,
                std::vector<int> & position) -> void
        {
            // potential new best
            if (c_popcount > result.size || (c_popcount == result.size && cost < result.cost)) {
                result.size = c_popcount;
                result.cost = cost;

                result.members.clear();
                for (int i = 0 ; i < graph.size() ; ++i)
                    if (c.test(i))
                        result.members.insert(order[i]);

                print_incumbent(params, c_popcount, cost, position);
            }
        }

        auto recurse(
                bool pass_2,
                FixedBitSet<size_> & c,
                FixedBitSet<size_> & p,
                LabelSet & u,
                std::vector<int> & position
                ) -> bool
        {
            expand(pass_2, c, p, u, position);
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
template auto parasols::lcco_max_labelled_clique<CCOPermutations::Sort>(const Graph &, const MaxLabelledCliqueParams &) -> MaxLabelledCliqueResult;

