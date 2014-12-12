/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <subgraph_isomorphism/naive_subgraph_isomorphism.hh>
#include <algorithm>

using namespace parasols;

namespace
{
    using Domains = std::vector<std::vector<bool> >;

    auto expand(const Graph & pattern, const Graph & target, const SubgraphIsomorphismParams & params, Domains & domains, unsigned long long & nodes) -> bool
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

                int popcount = std::count(domains.at(i).begin(), domains.at(i).end(), true);
                if (0 == popcount)
                    return false;
                else if (1 == popcount) {
                    int f_i = std::find(domains.at(i).begin(), domains.at(i).end(), true) - domains.at(i).begin();
                    for (int j = 0 ; j < pattern.size() ; ++j) {
                        if (i == j)
                            continue;

                        // all different
                        if (domains.at(j).at(f_i)) {
                            revise = true;
                            domains.at(j).at(f_i) = false;
                        }

                        if (pattern.adjacent(i, j)) {
                            // i--j => f(i)--f(j)
                            for (int v = 0 ; v < target.size() ; ++v) {
                                if (domains.at(j).at(v) && ! target.adjacent(f_i, v)) {
                                    domains.at(j).at(v) = false;
                                    revise = true;
                                }
                            }
                        }
                        else if (params.induced) {
                            // induced && i-/-j => f(i)-/-f(j)
                            for (int v = 0 ; v < target.size() ; ++v) {
                                if (domains.at(j).at(v) && target.adjacent(f_i, v)) {
                                    domains.at(j).at(v) = false;
                                    revise = true;
                                }
                            }
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
            if (domains.at(branch_on).at(v)) {
                auto domains_copy = domains;
                for (int w = 0 ; w < target.size() ; ++w)
                    domains_copy.at(branch_on).at(w) = (v == w);

                if (expand(pattern, target, params, domains_copy, nodes)) {
                    domains = domains_copy;
                    return true;
                }
            }
        }

        return false;
    }
}

auto parasols::naive_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    auto & pattern = graphs.first;
    auto & target = graphs.second;

    Domains domains(pattern.size());

    std::vector<int> pattern_degrees(pattern.size()), target_degrees(target.size());
    for (int i = 0 ; i < pattern.size() ; ++i)
        pattern_degrees.at(i) = pattern.degree(i);
    for (int i = 0 ; i < target.size() ; ++i)
        target_degrees.at(i) = target.degree(i);

    for (int i = 0 ; i < pattern.size() ; ++i) {
        domains.at(i).resize(target.size());
        for (int j = 0 ; j < target.size() ; ++j)
            if (target_degrees.at(j) >= pattern_degrees.at(i)) {
                if (pattern.adjacent(i, i) && ! target.adjacent(j, j)) {
                }
                else if (params.induced && target.adjacent(j, j) && ! pattern.adjacent(i, i)) {
                }
                else
                    domains.at(i).at(j) = true;
            }
    }

    SubgraphIsomorphismResult result;
    if (expand(pattern, target, params, domains, result.nodes)) {
        for (int i = 0 ; i < pattern.size() ; ++i)
            result.isomorphism.emplace(i, std::find(domains.at(i).begin(), domains.at(i).end(), true) - domains.at(i).begin());
    }

    return result;
}

