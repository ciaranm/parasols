/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/cco_max_clique.hh>
#include <max_clique/cco_base.hh>
#include <max_clique/print_incumbent.hh>

#include <graph/template_voodoo.hh>

#include <algorithm>
#include <thread>
#include <mutex>

using namespace parasols;

namespace
{
    template <CCOPermutations perm_, unsigned size_>
    struct CCO : CCOBase<perm_, size_, CCO<perm_, size_> >
    {
        using CCOBase<perm_, size_, CCO<perm_, size_> >::CCOBase;

        using CCOBase<perm_, size_, CCO<perm_, size_> >::graph;
        using CCOBase<perm_, size_, CCO<perm_, size_> >::params;
        using CCOBase<perm_, size_, CCO<perm_, size_> >::expand;
        using CCOBase<perm_, size_, CCO<perm_, size_> >::order;

        MaxCliqueResult result;

        auto run() -> MaxCliqueResult
        {
            result.size = params.initial_bound;

            FixedBitSet<size_> c; // current candidate clique
            c.resize(graph.size());

            FixedBitSet<size_> p; // potential additions
            p.resize(graph.size());
            p.set_all();

            std::vector<int> positions;
            positions.reserve(graph.size());
            positions.push_back(0);

            // go!
            expand(c, p, positions);

            // hack for enumerate
            if (params.enumerate)
                result.size = result.members.size();

            return result;
        }

        auto incremement_nodes() -> void
        {
            ++result.nodes;
        }

        auto recurse(
                FixedBitSet<size_> & c,
                FixedBitSet<size_> & p,
                std::vector<int> & position
                ) -> void
        {
            expand(c, p, position);
        }

        auto potential_new_best(
                unsigned c_popcount,
                const FixedBitSet<size_> & c,
                std::vector<int> & position) -> void
        {
            // potential new best
            if (c_popcount > result.size) {
                if (params.enumerate) {
                    ++result.result_count;
                    result.size = c_popcount - 1;
                }
                else
                    result.size = c_popcount;

                result.members.clear();
                for (int i = 0 ; i < graph.size() ; ++i)
                    if (c.test(i))
                        result.members.insert(order[i]);

                print_incumbent(params, c_popcount, position);
            }
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return result.size;
        }

        auto initialise_skip(
                int &,
                bool &,
                unsigned) -> void
        {
        }
    };
}

template <CCOPermutations perm_>
auto parasols::cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPerm<CCO, perm_>::template Type, MaxCliqueResult>(AllGraphSizes(), graph, params);
}

template auto parasols::cco_max_clique<CCOPermutations::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

