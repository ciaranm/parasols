/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <subgraph_isomorphism/b_subgraph_isomorphism.hh>

#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>

#include <algorithm>
#include <limits>

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
        const bool dds, nds;
        const int remaining_vars_cutoff;

        FixedBitGraph<size_> target_bitgraph;

        SGI(const Graph & t, const Graph & p, const SubgraphIsomorphismParams & a, bool d, bool n, bool c) :
            target(t),
            pattern(p),
            params(a),
            dds(d),
            nds(n),
            remaining_vars_cutoff(c ? 10 : std::numeric_limits<int>::max())
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
                            domains.at(j).values.unset(f_i);
                            if (! domains.at(j).revise) {
                                unsigned after = domains.at(j).values.popcount();
                                if (0 == after)
                                    return false;
                                else if (after < 2) {
                                    domains.at(j).revise = true;
                                    to_revise.push_back(j);
                                }
                            }
                        }

                        if (pattern.adjacent(i, j)) {
                            // i--j => f(i)--f(j)
                            unsigned before = domains.at(j).values.popcount();
                            target_bitgraph.intersect_with_row(f_i, domains.at(j).values);
                            unsigned after = domains.at(j).values.popcount();
                            if (0 == after)
                                return false;
                            else if (before != after && after < 2) {
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
                            else if (before != after && after < 2) {
                                if (! domains.at(j).revise) {
                                    domains.at(j).revise = true;
                                    to_revise.push_back(j);
                                }
                            }
                        }
                    }
                }
            }

            int branch_on = -1, branch_on_popcount = 0, remaining_vars = 0;
            for (int i = 0 ; i < pattern.size() ; ++i) {
                int popcount = domains.at(i).values.popcount();
                if (popcount > 1) {
                    ++remaining_vars;
                    if (-1 == branch_on || popcount < branch_on_popcount) {
                        branch_on = i;
                        branch_on_popcount = popcount;
                    }
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
                    if (depth >= allow_discrepancies_above && remaining_vars > remaining_vars_cutoff)
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

            if (! nds) {
                for (int i = 0 ; i < pattern.size() ; ++i) {
                    domains.at(i).values.resize(target.size());
                    domains.at(i).revise = true;
                    to_revise.push_back(i);
                    for (int j = 0 ; j < target.size() ; ++j)
                        if (target_degrees.at(j) >= pattern_degrees.at(i)) {
                            if (pattern.adjacent(i, i) && ! target.adjacent(j, j)) {
                            }
                            else if (params.induced && target.adjacent(j, j) && ! pattern.adjacent(i, i)) {
                            }
                            else
                                domains.at(i).values.set(j);
                        }
                }
            }
            else {
                std::vector<std::vector<int> > pattern_ndss(pattern.size()), target_ndss(target.size());
                for (int i = 0 ; i < pattern.size() ; ++i) {
                    for (int j = 0 ; j < pattern.size() ; ++j) {
                        if (pattern.adjacent(i, j))
                            pattern_ndss.at(i).push_back(pattern_degrees.at(j));
                    }
                    std::sort(pattern_ndss.at(i).begin(), pattern_ndss.at(i).end(), std::greater<int>());
                }

                for (int i = 0 ; i < target.size() ; ++i) {
                    for (int j = 0 ; j < target.size() ; ++j) {
                        if (target.adjacent(i, j))
                            target_ndss.at(i).push_back(target_degrees.at(j));
                    }
                    std::sort(target_ndss.at(i).begin(), target_ndss.at(i).end(), std::greater<int>());
                }

                for (int i = 0 ; i < pattern.size() ; ++i) {
                    domains.at(i).values.resize(target.size());
                    domains.at(i).revise = true;
                    to_revise.push_back(i);

                    for (int j = 0 ; j < target.size() ; ++j) {
                        if (target_ndss.at(j).size() >= pattern_ndss.at(i).size()) {
                            bool ok = true;
                            for (unsigned x = 0 ; x < pattern_ndss.at(i).size() ; ++x)
                                if (target_ndss.at(j).at(x) < pattern_ndss.at(i).at(x)) {
                                    ok = false;
                                    break;
                                }

                            if (ok)
                                domains.at(i).values.set(j);
                        }
                    }
                }
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
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, false, false, false);
}

auto parasols::bdds_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, false, false);
}

auto parasols::bddsc_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, false, true);
}

auto parasols::bnds_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, false, true, false);
}

auto parasols::bndsdds_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, true, false);
}

auto parasols::bndsddsc_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, true, true);
}

