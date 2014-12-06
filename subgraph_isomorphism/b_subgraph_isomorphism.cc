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
        bool revise;
        FixedBitSet<size_> values;
    };

    template <unsigned size_, typename>
    struct SGI
    {
        using Domains = std::vector<Domain<size_> >;
        using ToRevise = std::vector<int>;

        const Graph & target;
        const Graph & pattern;
        const SubgraphIsomorphismParams & params;
        const bool dds;

        FixedBitGraph<size_> target_bitgraph;

        SGI(const Graph & t, const Graph & p, const SubgraphIsomorphismParams & a, bool d) :
            target(t),
            pattern(p),
            params(a),
            dds(d)
        {
            target_bitgraph.resize(target.size());
            for (int i = 0 ; i < target.size() ; ++i)
                for (int j = 0 ; j < target.size() ; ++j)
                    if (target.adjacent(i, j))
                        target_bitgraph.add_edge(i, j);
        }

        auto expand(Domains & domains, ToRevise & to_revise, unsigned long long & nodes,
                unsigned depth, unsigned allow_discrepancies_above) -> bool
        {
            ++nodes;
            if (params.abort->load())
                return false;

            while (! to_revise.empty()) {
                if (params.abort->load())
                    return false;

                int i = to_revise.back();
                to_revise.pop_back();
                domains.at(i).revise = false;

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
                            if (! domains.at(j).revise) {
                                domains.at(j).revise = true;
                                to_revise.push_back(j);
                            }
                            domains.at(j).values.unset(f_i);
                        }

                        if (pattern.adjacent(i, j)) {
                            // i--j => f(i)--f(j)
                            unsigned before = domains.at(j).values.popcount();
                            target_bitgraph.intersect_with_row(f_i, domains.at(j).values);
                            unsigned after = domains.at(j).values.popcount();
                            if (0 == after)
                                return false;
                            else if (before != after) {
                                if (! domains.at(j).revise) {
                                    domains.at(j).revise = true;
                                    to_revise.push_back(j);
                                }
                            }
                        }
                        else if (params.induced) {
                            // induced && i-/-j => f(i)-/-f(j)
                            unsigned before = domains.at(j).values.popcount();
                            target_bitgraph.intersect_with_row_complement(f_i, domains.at(j).values);
                            unsigned after = domains.at(j).values.popcount();
                            if (0 == after)
                                return false;
                            else if (before != after) {
                                if (! domains.at(j).revise) {
                                    domains.at(j).revise = true;
                                    to_revise.push_back(j);
                                }
                            }
                        }
                    }
                }
            }

            int branch_on = -1, branch_on_popcount = 0;
            for (int i = 0 ; i < pattern.size() ; ++i) {
                int popcount = domains.at(i).values.popcount();
                if (popcount > 1 && (-1 == branch_on || popcount < branch_on_popcount)) {
                    branch_on = i;
                    branch_on_popcount = popcount;
                }
            }

            if (-1 == branch_on)
                return true;

            bool first = true;
            for (int v = 0 ; v < target.size() ; ++v) {
                if (domains.at(branch_on).values.test(v)) {
                    if (! (first && depth + 1 == allow_discrepancies_above)) {
                        auto new_domains = domains;
                        for (int w = 0 ; w < target.size() ; ++w) {
                            new_domains.at(branch_on).values.unset_all();
                            new_domains.at(branch_on).values.set(v);
                            new_domains.at(branch_on).revise = true;
                        }

                        ToRevise new_to_revise;
                        new_to_revise.push_back(branch_on);

                        if (expand(new_domains, new_to_revise, nodes, depth + 1, allow_discrepancies_above)) {
                            domains = std::move(new_domains);
                            return true;
                        }
                    }

                    first = false;
                    if (depth >= allow_discrepancies_above)
                        break;
                }
            }

            return false;
        }

        auto run() -> SubgraphIsomorphismResult
        {
            SubgraphIsomorphismResult result;

            Domains domains(pattern.size());
            ToRevise to_revise(pattern.size());

            std::vector<int> pattern_degrees(pattern.size()), target_degrees(target.size());
            for (int i = 0 ; i < pattern.size() ; ++i)
                pattern_degrees.at(i) = pattern.degree(i);
            for (int i = 0 ; i < target.size() ; ++i)
                target_degrees.at(i) = target.degree(i);

            for (int i = 0 ; i < pattern.size() ; ++i) {
                domains.at(i).values.resize(target.size());
                domains.at(i).revise = true;
                to_revise.push_back(i);
                for (int j = 0 ; j < target.size() ; ++j)
                    if (target_degrees.at(j) >= pattern_degrees.at(i))
                        domains.at(i).values.set(j);
            }

            if (! dds) {
                if (expand(domains, to_revise, result.nodes, 0, pattern.size() + 1)) {
                    for (int i = 0 ; i < pattern.size() ; ++i)
                        result.isomorphism.emplace(i, domains.at(i).values.first_set_bit());
                }
            }
            else {
                for (int k = 0 ; k < pattern.size() ; ++k) {
                    auto domains_copy = domains;
                    auto to_revise_copy = to_revise;
                    if (expand(domains_copy, to_revise_copy, result.nodes, 0, k)) {
                        for (int i = 0 ; i < pattern.size() ; ++i)
                            result.isomorphism.emplace(i, domains_copy.at(i).values.first_set_bit());
                        break;
                    }
                }
            }

            return result;
        }
    };
}

auto parasols::b_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, false);
}

auto parasols::bdds_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true);
}

