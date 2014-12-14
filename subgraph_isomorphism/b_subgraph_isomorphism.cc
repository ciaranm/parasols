/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <subgraph_isomorphism/b_subgraph_isomorphism.hh>

#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>

#include <algorithm>
#include <limits>

using namespace parasols;

namespace
{
    template <unsigned target_size_>
    struct Domain
    {
        bool uncommitted_singleton = false;
        bool committed = false;
        int popcount;

        FixedBitSet<target_size_> values;
    };

    template <unsigned target_size_, typename>
    struct SGI
    {
        using Domains = std::vector<Domain<target_size_> >;

        const Graph & target;
        const Graph & pattern;
        const SubgraphIsomorphismParams & params;
        const bool dds, nds;

        FixedBitGraph<target_size_> target_bitgraph;

        SGI(const Graph & t, const Graph & p, const SubgraphIsomorphismParams & a, bool d, bool n) :
            target(t),
            pattern(p),
            params(a),
            dds(d),
            nds(n)
        {
            target_bitgraph.resize(target.size());
            for (int i = 0 ; i < target.size() ; ++i)
                for (int j = 0 ; j < target.size() ; ++j)
                    if (target.adjacent(i, j))
                        target_bitgraph.add_edge(i, j);
        }

        auto filter(Domains & domains, int branch_point) -> bool
        {
            /* really a vector, but memory allocation is expensive. only needs
             * room for pattern.size() elements, but target_size_ will do. */
            std::array<int, target_size_ * bits_per_word> uncommitted_singletons;
            int uncommitted_singletons_end = 0;

            if (-1 == branch_point) {
                /* no branch point at the top of search, check all domains */
                for (int v = 0 ; v < pattern.size() ; ++v) {
                    domains.at(v).popcount = domains.at(v).values.popcount();
                    if (0 == domains.at(v).popcount)
                        return false;
                    else if (1 == domains.at(v).popcount) {
                        uncommitted_singletons[uncommitted_singletons_end++] = v;
                        domains.at(v).uncommitted_singleton = true;
                    }
                }
            }
            else {
                /* start by revising the branch point */
                uncommitted_singletons[uncommitted_singletons_end++] = branch_point;
                domains.at(branch_point).uncommitted_singleton = true;
            }

            /* commit all uncommitted singletons */
            while (uncommitted_singletons_end > 0) {
                if (params.abort->load())
                    return false;

                /* v has only one value in its domain, but is not committed */
                int v = uncommitted_singletons[--uncommitted_singletons_end];
                domains.at(v).uncommitted_singleton = false;
                domains.at(v).committed = true;

                /* what's it mapped to? */
                int f_v = domains.at(v).values.first_set_bit();

                /* for each other domain... */
                for (int w = 0 ; w < pattern.size() ; ++w) {
                    if (v == w || domains.at(w).committed)
                        continue;

                    bool w_domain_potentially_changed = false;

                    if (domains.at(w).values.test(f_v)) {
                        /* no other domain can be mapped to f_v */
                        domains.at(w).values.unset(f_v);
                        w_domain_potentially_changed = true;
                    }

                    if (pattern.adjacent(v, w)) {
                        /* v is adjacent to w, so w can only be mapped to things adjacent to f_v */
                        target_bitgraph.intersect_with_row(f_v, domains.at(w).values);
                        w_domain_potentially_changed = true;
                    }
                    else if (params.induced) {
                        /* v is nonadjacent to w, so w can only be mapped to things nonadjacent to f_v */
                        target_bitgraph.intersect_with_row_complement(f_v, domains.at(w).values);
                        w_domain_potentially_changed = true;
                    }

                    if (w_domain_potentially_changed) {
                        /* zero or one values left in the domain? */
                        domains.at(w).popcount = domains.at(w).values.popcount();
                        switch (domains.at(w).popcount) {
                            case 0:
                                return false;

                            case 1:
                                if (! domains.at(w).uncommitted_singleton) {
                                    domains.at(w).uncommitted_singleton = true;
                                    uncommitted_singletons[uncommitted_singletons_end++] = w;
                                }
                                break;
                        }
                    }
                }
            }

            return true;
        }

        auto expand(Domains & domains, int branch_point, unsigned long long & nodes,
                unsigned depth, unsigned allow_discrepancies_above) -> bool
        {
            ++nodes;
            if (params.abort->load())
                return false;

            if (! filter(domains, branch_point))
                return false;

            int branch_on = -1, branch_on_popcount = 0;
            for (int i = 0 ; i < pattern.size() ; ++i) {
                if (! domains.at(i).committed) {
                    int popcount = domains.at(i).popcount;
                    if (popcount > 1) {
                        if (-1 == branch_on || popcount < branch_on_popcount) {
                            branch_on = i;
                            branch_on_popcount = popcount;
                        }
                    }
                }
            }

            if (-1 == branch_on)
                return true;

            bool first = true;
            int v;
            while (((v = domains.at(branch_on).values.first_set_bit())) != -1) {
                domains.at(branch_on).values.unset(v);
                if (! (first && depth + 1 == allow_discrepancies_above)) {
                    auto new_domains = domains;
                    for (int w = 0 ; w < target.size() ; ++w) {
                        new_domains.at(branch_on).values.unset_all();
                        new_domains.at(branch_on).values.set(v);
                        new_domains.at(branch_on).uncommitted_singleton = true;
                    }

                    if (expand(new_domains, branch_on, nodes, depth + 1, allow_discrepancies_above)) {
                        domains = std::move(new_domains);
                        return true;
                    }
                }

                first = false;
                if (depth >= allow_discrepancies_above)
                    break;
            }

            return false;
        }

        auto run() -> SubgraphIsomorphismResult
        {
            SubgraphIsomorphismResult result;

            if (pattern.size() > target.size()) {
                /* some of our fixed size data structures will throw a hissy
                 * fit. check this early. */
                return result;
            }

            Domains domains(pattern.size());

            std::vector<int> pattern_degrees(pattern.size()), target_degrees(target.size());
            for (int i = 0 ; i < pattern.size() ; ++i)
                pattern_degrees.at(i) = pattern.degree(i);
            for (int i = 0 ; i < target.size() ; ++i)
                target_degrees.at(i) = target.degree(i);

            if (! nds) {
                for (int i = 0 ; i < pattern.size() ; ++i) {
                    domains.at(i).values.resize(target.size());
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
                if (expand(domains, -1, result.nodes, 0, pattern.size() + 1)) {
                    for (int i = 0 ; i < pattern.size() ; ++i)
                        result.isomorphism.emplace(i, domains.at(i).values.first_set_bit());
                }
            }
            else {
                for (int k = 0 ; k < pattern.size() ; ++k) {
                    auto domains_copy = domains;
                    if (expand(domains_copy, -1, result.nodes, 0, k)) {
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
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, false, false);
}

auto parasols::bdds_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, false);
}

auto parasols::bnds_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, false, true);
}

auto parasols::bndsdds_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<SGI, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, true);
}

