/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_biclique/cpo_max_biclique.hh>
#include <max_biclique/cpo_base.hh>
#include <max_biclique/print_incumbent.hh>

#include <graph/template_voodoo.hh>

#include <algorithm>
#include <thread>
#include <mutex>

using namespace parasols;

namespace
{
    template <CCOPermutations perm_, BicliqueSymmetryRemoval sym_, unsigned size_, typename VertexType_>
    struct CPO : CPOBase<perm_, sym_, size_, VertexType_, CPO<perm_, sym_, size_, VertexType_> >
    {
        using Base = CPOBase<perm_, sym_, size_, VertexType_, CPO<perm_, sym_, size_, VertexType_> >;

        using Base::CPOBase;

        using Base::graph;
        using Base::params;
        using Base::expand;
        using Base::order;
        using Base::colour_class_order;

        MaxBicliqueResult result;

        std::list<std::set<int> > previouses;

        auto run() -> MaxBicliqueResult
        {
            result.size = params.initial_bound;

            std::vector<unsigned> ca, cb;
            ca.reserve(graph.size());
            cb.reserve(graph.size());

            FixedBitSet<size_> pa, pb; // potential additions
            pa.resize(graph.size());
            pa.set_all();
            pb.resize(graph.size());
            pb.set_all();

            FixedBitSet<size_> sym_skip;
            sym_skip.resize(graph.size());

            std::vector<int> positions;
            positions.reserve(graph.size());
            positions.push_back(0);

            // initial colouring
            std::array<VertexType_, size_ * bits_per_word> initial_p_order;
            std::array<VertexType_, size_ * bits_per_word> initial_bound;
            colour_class_order(SelectColourClassOrderOverload<perm_>(), pa, initial_p_order, initial_bound);

            // go!
            expand(ca, cb, pa, pb, sym_skip, initial_p_order, initial_bound, positions);

            return result;
        }

        auto increment_nodes() -> void
        {
            ++result.nodes;
        }

        auto recurse(
                std::vector<unsigned> & ca,
                std::vector<unsigned> & cb,
                FixedBitSet<size_> & pa,
                FixedBitSet<size_> & pb,
                FixedBitSet<size_> & sym_skip,
                const std::array<VertexType_, size_ * bits_per_word> & pa_order,
                const std::array<VertexType_, size_ * bits_per_word> & pa_bounds,
                std::vector<int> & position
                ) -> bool
        {
            expand(ca, cb, pa, pb, sym_skip, pa_order, pa_bounds, position);
            return true;
        }

        auto potential_new_best(
                const std::vector<unsigned> & ca,
                const std::vector<unsigned> & cb,
                const std::vector<int> & position) -> void
        {
            if (ca.size() == cb.size() && ca.size() > result.size) {
                result.size = ca.size();

                // depermute to get result
                result.members_a.clear();
                for (auto & v : ca)
                    result.members_a.insert(order[v]);

                result.members_b.clear();
                for (auto & v : cb)
                    result.members_b.insert(order[v]);

                print_incumbent(params, result.size, position);
            }
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return result.size;
        }

        auto get_skip(unsigned, unsigned, int &, bool &) -> void
        {
        }
    };
}

template <CCOPermutations perm_, BicliqueSymmetryRemoval sym_>
auto parasols::cpo_max_biclique(const Graph & graph, const MaxBicliqueParams & params) -> MaxBicliqueResult
{
    return select_graph_size<ApplyPermSym<CPO, perm_, sym_>::template Type, MaxBicliqueResult>(AllGraphSizes(), graph, params);
}

template auto parasols::cpo_max_biclique<CCOPermutations::None, BicliqueSymmetryRemoval::None>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;
template auto parasols::cpo_max_biclique<CCOPermutations::Defer1, BicliqueSymmetryRemoval::None>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;

template auto parasols::cpo_max_biclique<CCOPermutations::None, BicliqueSymmetryRemoval::Remove>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;
template auto parasols::cpo_max_biclique<CCOPermutations::Defer1, BicliqueSymmetryRemoval::Remove>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;

template auto parasols::cpo_max_biclique<CCOPermutations::None, BicliqueSymmetryRemoval::Skip>(const Graph &, const MaxBicliqueParams &) -> MaxBicliqueResult;

