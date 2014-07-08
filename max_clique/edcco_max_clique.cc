/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/edcco_max_clique.hh>
#include <max_clique/cco_base.hh>
#include <max_clique/print_incumbent.hh>

#include <graph/template_voodoo.hh>
#include <graph/merge_cliques.hh>

#include <algorithm>
#include <thread>
#include <mutex>

using namespace parasols;

namespace
{
    template <CCOPermutations perm_, CCOInference inference_, CCOMerge merge_, unsigned size_, typename VertexType_>
    struct EDCCO : CCOBase<perm_, inference_, size_, VertexType_, EDCCO<perm_, inference_, merge_, size_, VertexType_> >
    {
        using Base = CCOBase<perm_, inference_, size_, VertexType_, EDCCO<perm_, inference_, merge_, size_, VertexType_> >;

        using Base::original_graph;
        using Base::graph;
        using Base::params;
        using Base::expand;
        using Base::order;
        using Base::colour_class_order;

        MaxCliqueResult result;

        std::list<std::set<int> > previouses;

        int discrepancies;
        int increasing;

        EDCCO(const Graph & g, const MaxCliqueParams & p, int i) :
            Base(g, p),
            increasing(i)
        {
        }

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
            result.initial_colour_bound = initial_colours[graph.size() - 1];

            // initial discrepancy search
            for (discrepancies = 0 ; discrepancies < (0 == increasing ? 4 : increasing)  ; ++discrepancies) {
                auto d_c = c;
                auto d_p = p;
                auto d_positions = positions;
                print_position(params, "doing discrepancy pass " + std::to_string(discrepancies), d_positions);
                expand(d_c, d_p, initial_p_order, initial_colours, d_positions);
            }

            // remaining full search
            discrepancies = -1;
            print_position(params, "doing full search", positions);
            expand(c, p, initial_p_order, initial_colours, positions);

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
            switch (merge_) {
                case CCOMerge::None:
                    if (c.size() > result.size) {
                        result.size = c.size();

                        result.members.clear();
                        for (auto & v : c)
                            result.members.insert(order[v]);

                        print_incumbent(params, c.size(), position);
                    }
                    break;

                case CCOMerge::Previous:
                    {
                        std::set<int> new_members;
                        for (auto & v : c)
                            new_members.insert(order[v]);

                        auto merged = merge_cliques([&] (int a, int b) { return original_graph.adjacent(a, b); }, result.members, new_members);
                        if (merged.size() > result.size) {
                            result.members = merged;
                            result.size = result.members.size();
                            print_incumbent(params, result.size, position);
                        }
                    }
                    break;

                case CCOMerge::All:
                    {
                        std::set<int> new_members;
                        for (auto & v : c)
                            new_members.insert(order[v]);

                        if (previouses.empty()) {
                            result.members = new_members;
                            result.size = result.members.size();
                            previouses.push_back(result.members);
                            print_incumbent(params, result.size, position);
                        }
                        else
                            for (auto & p : previouses) {
                                auto merged = merge_cliques([&] (int a, int b) { return original_graph.adjacent(a, b); }, p, new_members);

                                if (merged.size() > result.size) {
                                    result.members = merged;
                                    result.size = result.members.size();
                                    previouses.push_back(result.members);
                                    print_incumbent(params, result.size, position);
                                }
                            }

                        previouses.push_back(result.members);
                        print_position(params, "previouses is now " + std::to_string(previouses.size()), position);
                    }
                    break;
            }
        }

        auto get_best_anywhere_value() -> unsigned
        {
            return result.size;
        }

        auto get_skip_and_stop(unsigned c_size, int & skip, int & stop, bool & keep_going) -> void
        {
            if (-1 != discrepancies) {
               if (c_size > unsigned(discrepancies)) {
                   skip = 0;
                   keep_going = false;
               }
               else
                   stop = graph.size() >> c_size;
            }
        }
    };
}

template <CCOPermutations perm_, CCOInference inference_, CCOMerge merge_>
auto parasols::edcco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPermInferenceMerge<EDCCO, perm_, inference_, merge_>::template Type, MaxCliqueResult>(
            AllGraphSizes(), graph, params, 0);
}

template <CCOPermutations perm_, CCOInference inference_, CCOMerge merge_>
auto parasols::id1cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPermInferenceMerge<EDCCO, perm_, inference_, merge_>::template Type, MaxCliqueResult>(
            AllGraphSizes(), graph, params, 1);
}

template <CCOPermutations perm_, CCOInference inference_, CCOMerge merge_>
auto parasols::id2cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPermInferenceMerge<EDCCO, perm_, inference_, merge_>::template Type, MaxCliqueResult>(
            AllGraphSizes(), graph, params, 2);
}

template <CCOPermutations perm_, CCOInference inference_, CCOMerge merge_>
auto parasols::id3cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPermInferenceMerge<EDCCO, perm_, inference_, merge_>::template Type, MaxCliqueResult>(
            AllGraphSizes(), graph, params, 3);
}

template <CCOPermutations perm_, CCOInference inference_, CCOMerge merge_>
auto parasols::id4cco_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    return select_graph_size<ApplyPermInferenceMerge<EDCCO, perm_, inference_, merge_>::template Type, MaxCliqueResult>(
            AllGraphSizes(), graph, params, 4);
}

template auto parasols::edcco_max_clique<CCOPermutations::None, CCOInference::None, CCOMerge::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::edcco_max_clique<CCOPermutations::Defer1, CCOInference::None, CCOMerge::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::edcco_max_clique<CCOPermutations::Sort, CCOInference::None, CCOMerge::None>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::edcco_max_clique<CCOPermutations::None, CCOInference::None, CCOMerge::Previous>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::edcco_max_clique<CCOPermutations::Defer1, CCOInference::None, CCOMerge::Previous>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::edcco_max_clique<CCOPermutations::Sort, CCOInference::None, CCOMerge::Previous>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::edcco_max_clique<CCOPermutations::None, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::edcco_max_clique<CCOPermutations::Defer1, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::edcco_max_clique<CCOPermutations::Sort, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::id1cco_max_clique<CCOPermutations::None, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::id1cco_max_clique<CCOPermutations::Defer1, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::id2cco_max_clique<CCOPermutations::None, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::id2cco_max_clique<CCOPermutations::Defer1, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::id3cco_max_clique<CCOPermutations::None, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::id3cco_max_clique<CCOPermutations::Defer1, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

template auto parasols::id4cco_max_clique<CCOPermutations::None, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::id4cco_max_clique<CCOPermutations::Defer1, CCOInference::None, CCOMerge::All>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

