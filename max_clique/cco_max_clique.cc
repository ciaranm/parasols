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
    template <CCOPermutations perm_, CCOInference inference_, unsigned size_, typename VertexType_>
    struct CCO : CCOBase<perm_, inference_, size_, VertexType_, CCO<perm_, inference_, size_, VertexType_> >
    {
        using CCOBase<perm_, inference_, size_, VertexType_, CCO<perm_, inference_, size_, VertexType_> >::CCOBase;

        using CCOBase<perm_, inference_, size_, VertexType_, CCO<perm_, inference_, size_, VertexType_> >::graph;
        using CCOBase<perm_, inference_, size_, VertexType_, CCO<perm_, inference_, size_, VertexType_> >::params;
        using CCOBase<perm_, inference_, size_, VertexType_, CCO<perm_, inference_, size_, VertexType_> >::expand;
        using CCOBase<perm_, inference_, size_, VertexType_, CCO<perm_, inference_, size_, VertexType_> >::order;
        using CCOBase<perm_, inference_, size_, VertexType_, CCO<perm_, inference_, size_, VertexType_> >::colour_class_order;

        MaxCliqueResult result;

        auto run() -> MaxCliqueResult
        {
            result.size = params.initial_bound;

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
            expand(c, p, initial_p_order, initial_colours, positions);

            // hack for enumerate
            if (params.enumerate)
                result.size = result.members.size();

            return result;
        }

        auto increment_nodes() -> void
        {
            ++result.nodes;
        }

        auto recurse(
                std::vector<unsigned> & c,                       // current candidate clique
                FixedBitSet<size_> & p,
                const std::array<VertexType_, size_ * bits_per_word> & p_order,
                const std::array<VertexType_, size_ * bits_per_word> & colours,
                std::vector<int> & position
                ) -> bool
        {
            expand(c, p, p_order, colours, position);
            return true;
        }

        auto potential_new_best(
                const std::vector<unsigned> & c,
                const std::vector<int> & position) -> void
        {
            // potential new best
            if (c.size() > result.size) {
                if (params.enumerate) {
                    ++result.result_count;
                    result.size = c.size() - 1;
                }
                else
                    result.size = c.size();

                result.members.clear();
                for (auto & v : c)
                    result.members.insert(order[v]);

                print_incumbent(params, c.size(), position);
            }
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return result.size;
        }

        auto get_skip(unsigned, int &, bool &) -> void
        {
        }
    };
}

template <CCOPermutations perm_, CCOInference inference_>
auto parasols::cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPermInference<CCO, perm_, inference_>::template Type, MaxCliqueResult>(
            AllGraphSizes(), graph, params);
}

template auto parasols::cco_max_clique<CCOPermutations::None, CCOInference::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1, CCOInference::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort, CCOInference::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::cco_max_clique<CCOPermutations::None, CCOInference::GlobalDomination>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1, CCOInference::GlobalDomination>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort, CCOInference::GlobalDomination>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::cco_max_clique<CCOPermutations::None, CCOInference::LazyGlobalDomination>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Defer1, CCOInference::LazyGlobalDomination>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::cco_max_clique<CCOPermutations::Sort, CCOInference::LazyGlobalDomination>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
