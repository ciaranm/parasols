/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <subgraph_isomorphism/b_subgraph_isomorphism.hh>

#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>

#include <algorithm>

using namespace parasols;

namespace
{
    template <unsigned size_>
    struct Domain
    {
        FixedBitSet<size_> values;
    };

    template <unsigned size_, typename>
    struct SGI
    {
        using Domains = std::vector<Domain<size_> >;

        const Graph & target;
        const Graph & pattern;
        const SubgraphIsomorphismParams & params;

        FixedBitGraph<size_> target_bitgraph;

        SGI(const Graph & t, const Graph & p, const SubgraphIsomorphismParams & a) :
            target(t),
            pattern(p),
            params(a)
        {
            target_bitgraph.resize(target.size());
            for (int i = 0 ; i < target.size() ; ++i)
                for (int j = 0 ; j < target.size() ; ++j)
                    if (target.adjacent(i, j))
                        target_bitgraph.add_edge(i, j);
        }

        auto expand(Domains & domains, unsigned long long & nodes) -> bool
        {
            ++nodes;
            if (params.abort->load())
                return false;

            bool revise = true;
            int branch_on, branch_on_popcount = 0;
            while (revise) {
                revise = false;
                branch_on = -1;
                for (int i = 0 ; i < pattern.size() ; ++i) {
                    if (params.abort->load())
                        return false;

                    int popcount = domains.at(i).values.popcount();
                    if (0 == popcount)
                        return false;
                    else if (1 == popcount) {
                        int f_i = domains.at(i).values.first_set_bit();
                        for (int j = 0 ; j < pattern.size() ; ++j) {
                            if (i == j)
                                continue;

                            // all different
                            if (domains.at(j).values.test(f_i)) {
                                revise = true;
                                domains.at(j).values.unset(f_i);
                            }

                            if (pattern.adjacent(i, j)) {
                                // i--j => f(i)--f(j)
                                unsigned before = domains.at(j).values.popcount();
                                target_bitgraph.intersect_with_row(f_i, domains.at(j).values);
                                if (before != domains.at(j).values.popcount())
                                    revise = true;
                            }
                            else if (params.induced) {
                                // induced && i-/-j => f(i)-/-f(j)
                                unsigned before = domains.at(j).values.popcount();
                                target_bitgraph.intersect_with_row_complement(f_i, domains.at(j).values);
                                if (before != domains.at(j).values.popcount())
                                    revise = true;
                            }
                        }
                    }
                    else if (-1 == branch_on || popcount < branch_on_popcount) {
                        branch_on = i;
                        branch_on_popcount = popcount;
                    }
                }
            }

            if (-1 == branch_on)
                return true;

            for (int v = 0 ; v < target.size() ; ++v) {
                if (domains.at(branch_on).values.test(v)) {
                    auto domains_copy = domains;
                    for (int w = 0 ; w < target.size() ; ++w) {
                        domains_copy.at(branch_on).values.unset_all();
                        domains_copy.at(branch_on).values.set(v);
                    }

                    if (expand(domains_copy, nodes)) {
                        domains = domains_copy;
                        return true;
                    }
                }
            }

            return false;
        }

        auto run() -> SubgraphIsomorphismResult
        {
            SubgraphIsomorphismResult result;

            Domains domains(pattern.size());

            std::vector<int> pattern_degrees(pattern.size()), target_degrees(target.size());
            for (int i = 0 ; i < pattern.size() ; ++i)
                pattern_degrees.at(i) = pattern.degree(i);
            for (int i = 0 ; i < target.size() ; ++i)
                target_degrees.at(i) = target.degree(i);

            for (int i = 0 ; i < pattern.size() ; ++i) {
                domains.at(i).values.resize(target.size());
                for (int j = 0 ; j < target.size() ; ++j)
                    if (target_degrees.at(j) >= pattern_degrees.at(i))
                        domains.at(i).values.set(j);
            }

            if (expand(domains, result.nodes)) {
                for (int i = 0 ; i < pattern.size() ; ++i)
                    result.isomorphism.emplace(i, domains.at(i).values.first_set_bit());
            }

            return result;
        }
    };
}

auto parasols::b_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params);
}

