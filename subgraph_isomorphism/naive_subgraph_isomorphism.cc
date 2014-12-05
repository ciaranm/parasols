/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <subgraph_isomorphism/naive_subgraph_isomorphism.hh>
#include <set>

using namespace parasols;

namespace
{
    using Domains = std::vector<std::set<int> >;

    auto expand(const Graph & pattern, const Graph & target, const SubgraphIsomorphismParams & params, Domains & domains, unsigned long long & nodes) -> bool
    {
        ++nodes;
        if (params.abort->load())
            return false;

        bool revise = true;
        int branch_on;
        while (revise) {
            revise = false;
            branch_on = -1;
            for (int i = 0 ; i < pattern.size() ; ++i)
                if (domains.at(i).empty())
                    return false;
                else if (domains.at(i).size() == 1) {
                    int f_i = *domains.at(i).begin();
                    for (int j = 0 ; j < pattern.size() ; ++j) {
                        if (i == j)
                            continue;

                        // all different
                        if (domains.at(j).count(f_i)) {
                            revise = true;
                            domains.at(j).erase(f_i);
                        }

                        if (pattern.adjacent(i, j)) {
                            // i--j => f(i)--f(j)
                            for (auto v = domains.at(j).begin(), v_end = domains.at(j).end() ; v != v_end ; ) {
                                if (target.adjacent(f_i, *v))
                                    ++v;
                                else {
                                    domains.at(j).erase(v++);
                                    revise = true;
                                }
                            }
                        }
                        else if (params.induced) {
                            // induced && i-/-j => f(i)-/-f(j)
                            for (auto v = domains.at(j).begin(), v_end = domains.at(j).end() ; v != v_end ; ) {
                                if (! target.adjacent(f_i, *v))
                                    ++v;
                                else {
                                    domains.at(j).erase(v++);
                                    revise = true;
                                }
                            }
                        }
                    }
                }
                else
                    branch_on = i;
        }

        if (-1 == branch_on)
            return true;

        for (auto & v : domains.at(branch_on)) {
            auto domains_copy = domains;
            domains_copy.at(branch_on) = { v };
            if (expand(pattern, target, params, domains_copy, nodes)) {
                domains = domains_copy;
                return true;
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

    for (int i = 0 ; i < pattern.size() ; ++i)
        for (int j = 0 ; j < target.size() ; ++j)
            if (target_degrees.at(j) >= pattern_degrees.at(i))
                domains.at(i).insert(j);

    SubgraphIsomorphismResult result;
    if (expand(pattern, target, params, domains, result.nodes)) {
        for (int i = 0 ; i < pattern.size() ; ++i)
            result.isomorphism.emplace(i, *domains.at(i).begin());
    }

    return result;
}

