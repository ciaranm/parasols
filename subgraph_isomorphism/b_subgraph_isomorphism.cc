/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <subgraph_isomorphism/b_subgraph_isomorphism.hh>

#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>
#include <graph/kneighbours.hh>

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
        bool on_edge_stack = false;
        int popcount;

        FixedBitSet<target_size_> values;
    };

    template <unsigned target_size_, typename, bool p3_, bool sum_domains_>
    struct SGI
    {
        using Domains = std::vector<Domain<target_size_> >;

        const Graph & target;
        const Graph & pattern;
        const SubgraphIsomorphismParams & params;
        const bool nds, ndsr, x2, x22, lv, x3;

        FixedBitGraph<target_size_> target_bitgraph;

        Graph pattern_paths2;
        FixedBitGraph<target_size_> target_paths2_bitgraph;
        Graph pattern_paths3;
        FixedBitGraph<target_size_> target_paths3_bitgraph;

        SGI(const Graph & t, const Graph & p, const SubgraphIsomorphismParams & a, bool n, bool r, bool x, bool xx, bool v, bool xxx) :
            target(t),
            pattern(p),
            params(a),
            nds(n),
            ndsr(r),
            x2(x),
            x22(xx),
            lv(v),
            x3(xxx)
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

                /* how many unassigned neighbours do we have, and what is their domain? */
                unsigned unassigned_neighbours = 0;
                FixedBitSet<target_size_> unassigned_neighbours_domains_union;
                unassigned_neighbours_domains_union.resize(target_bitgraph.size());

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

                        if (lv) {
                            ++unassigned_neighbours;
                            unassigned_neighbours_domains_union.union_with(domains.at(w).values);
                        }
                    }
                    else if (params.induced) {
                        /* v is nonadjacent to w, so w can only be mapped to things nonadjacent to f_v */
                        target_bitgraph.intersect_with_row_complement(f_v, domains.at(w).values);
                        w_domain_potentially_changed = true;
                    }

                    if (p3_) {
                        if (pattern_paths2.adjacent(v, w)) {
                            /* v is distance 2 away from w, so w can only be mapped to things distance 2 away from f_v */
                            target_paths2_bitgraph.intersect_with_row(f_v, domains.at(w).values);
                            w_domain_potentially_changed = true;
                        }

                        if (((! x2) || (x2 && x22)) && pattern_paths3.adjacent(v, w)) {
                            /* v is distance 2 away from w, so w can only be mapped to things distance 2 away from f_v */
                            target_paths3_bitgraph.intersect_with_row(f_v, domains.at(w).values);
                            w_domain_potentially_changed = true;
                        }
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

                /* enough values left in neighbourhood domains? */
                if (lv && 0 != unassigned_neighbours) {
                    if (unassigned_neighbours > unassigned_neighbours_domains_union.popcount())
                        return false;
                }
            }

            if (sum_domains_) {
                FixedBitSet<target_size_> domains_union;
                domains_union.resize(pattern.size());
                for (auto & d : domains)
                    domains_union.union_with(d.values);

                if (unsigned(domains_union.popcount()) < unsigned(pattern.size()))
                    return false;
            }

            return true;
        }

        auto expand(Domains & domains, int branch_point, unsigned long long & nodes) -> bool
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

            int v;
            while (((v = domains.at(branch_on).values.first_set_bit())) != -1) {
                domains.at(branch_on).values.unset(v);

                auto new_domains = domains;
                for (int w = 0 ; w < target.size() ; ++w) {
                    new_domains.at(branch_on).values.unset_all();
                    new_domains.at(branch_on).values.set(v);
                    new_domains.at(branch_on).uncommitted_singleton = true;
                }

                if (expand(new_domains, branch_on, nodes)) {
                    domains = std::move(new_domains);
                    return true;
                }
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

            if (! nds) {
                std::vector<int> pattern_degrees(pattern.size()), target_degrees(target.size());
                for (int i = 0 ; i < pattern.size() ; ++i)
                    pattern_degrees.at(i) = pattern.degree(i);
                for (int i = 0 ; i < target.size() ; ++i)
                    target_degrees.at(i) = target.degree(i);

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
                unsigned remaining_target_vertices = target.size();
                FixedBitSet<target_size_> allowed_target_vertices;
                allowed_target_vertices.resize(target.size());
                allowed_target_vertices.set_all();

                while (true) {
                    std::vector<int> pattern_degrees(pattern.size()), target_degrees(target.size());
                    for (int i = 0 ; i < pattern.size() ; ++i)
                        pattern_degrees.at(i) = pattern.degree(i);
                    for (int i = 0 ; i < target.size() ; ++i) {
                        FixedBitSet<target_size_> remaining = allowed_target_vertices;
                        target_bitgraph.intersect_with_row(i, remaining);
                        target_degrees.at(i) = remaining.popcount();
                    }

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
                        domains.at(i).values.unset_all();
                        domains.at(i).values.resize(target.size());

                        for (int j = 0 ; j < target.size() ; ++j) {
                            if (! allowed_target_vertices.test(j)) {
                            }
                            else if (pattern.adjacent(i, i) && ! target.adjacent(j, j)) {
                            }
                            else if (params.induced && target.adjacent(j, j) && ! pattern.adjacent(i, i)) {
                            }
                            else if (target_ndss.at(j).size() >= pattern_ndss.at(i).size()) {
                                bool ok = true;
                                for (unsigned x = 0 ; x < pattern_ndss.at(i).size() ; ++x) {
                                    if (target_ndss.at(j).at(x) < pattern_ndss.at(i).at(x)) {
                                        ok = false;
                                        break;
                                    }
                                }

                                if (ok)
                                    domains.at(i).values.set(j);
                            }
                        }
                    }

                    if (! ndsr)
                        break;

                    FixedBitSet<target_size_> domains_union;
                    domains_union.resize(pattern.size());
                    for (auto & d : domains)
                        domains_union.union_with(d.values);

                    unsigned domains_union_popcount = domains_union.popcount();
                    if (domains_union_popcount < unsigned(pattern.size())) {
                        return result;
                    }
                    else if (domains_union_popcount == remaining_target_vertices)
                        break;

                    allowed_target_vertices.intersect_with(domains_union);
                    remaining_target_vertices = allowed_target_vertices.popcount();
                }

                if (p3_ && ! x2 && ! x3) {
                    pattern_paths2.resize(pattern.size());

                    KNeighbours pattern_distances2(pattern, 2, nullptr);
                    for (int v = 0 ; v < pattern.size() ; ++v)
                        for (int w = 0 ; w < pattern.size() ; ++w)
                            if (pattern_distances2.vertices[v].distances[w].distance > 0)
                                pattern_paths2.add_edge(v, w);

                    target_paths2_bitgraph.resize(target.size());

                    KNeighbours target_distances2(target, 2, nullptr);
                    for (int v = 0 ; v < target.size() ; ++v)
                        for (int w = 0 ; w < target.size() ; ++w)
                            if (target_distances2.vertices[v].distances[w].distance > 0)
                                target_paths2_bitgraph.add_edge(v, w);

                    pattern_paths3.resize(pattern.size());

                    KNeighbours pattern_distances3(pattern, 3, nullptr);
                    for (int v = 0 ; v < pattern.size() ; ++v)
                        for (int w = 0 ; w < pattern.size() ; ++w)
                            if (pattern_distances3.vertices[v].distances[w].distance > 0)
                                pattern_paths3.add_edge(v, w);

                    target_paths3_bitgraph.resize(target.size());

                    KNeighbours target_distances3(target, 3, nullptr);
                    for (int v = 0 ; v < target.size() ; ++v)
                        for (int w = 0 ; w < target.size() ; ++w)
                            if (target_distances3.vertices[v].distances[w].distance > 0)
                                target_paths3_bitgraph.add_edge(v, w);
                }
                else if (p3_ && x2) {
                    pattern_paths2.resize(pattern.size());
                    pattern_paths3.resize(pattern.size());

                    for (int v = 0 ; v < pattern.size() ; ++v) {
                        for (int c = 0 ; c < pattern.size() ; ++c) {
                            if (pattern.adjacent(c, v)) {
                                for (int w = 0 ; w < v ; ++w) {
                                    if (pattern.adjacent(c, w)) {
                                        if (x22 && pattern_paths2.adjacent(v, w))
                                            pattern_paths3.add_edge(v, w);
                                        else
                                            pattern_paths2.add_edge(v, w);
                                    }
                                }
                            }
                        }
                    }

                    target_paths2_bitgraph.resize(target.size());
                    target_paths3_bitgraph.resize(target.size());

                    for (int v = 0 ; v < target.size() ; ++v) {
                        for (int c = 0 ; c < target.size() ; ++c) {
                            if (target.adjacent(c, v)) {
                                for (int w = 0 ; w < v ; ++w) {
                                    if (target.adjacent(c, w)) {
                                        if (x22 && target_paths2_bitgraph.adjacent(v, w))
                                            target_paths3_bitgraph.add_edge(v, w);
                                        else
                                            target_paths2_bitgraph.add_edge(v, w);
                                    }
                                }
                            }
                        }
                    }
                }
                else if (p3_ && x3) {
                    pattern_paths2.resize(pattern.size());
                    pattern_paths3.resize(pattern.size());

                    for (int v = 0 ; v < pattern.size() ; ++v) {
                        for (int c = 0 ; c < pattern.size() ; ++c) {
                            if (pattern.adjacent(c, v)) {
                                for (int w = 0 ; w < v ; ++w) {
                                    if (pattern.adjacent(c, w))
                                        pattern_paths2.add_edge(v, w);
                                }
                            }
                        }
                    }

                    for (int v = 0 ; v < pattern.size() ; ++v) {
                        for (int c = 0 ; c < pattern.size() ; ++c) {
                            if (pattern.adjacent(c, v)) {
                                for (int d = 0 ; d < pattern.size() ; ++d) {
                                    if (d != v && pattern.adjacent(c, d)) {
                                        for (int w = 0 ; w < v ; ++w) {
                                            if (w != c && pattern.adjacent(d, w))
                                                pattern_paths3.add_edge(v, w);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    target_paths2_bitgraph.resize(target.size());
                    target_paths3_bitgraph.resize(target.size());

                    for (int v = 0 ; v < target.size() ; ++v) {
                        for (int c = 0 ; c < target.size() ; ++c) {
                            if (target.adjacent(c, v)) {
                                for (int w = 0 ; w < v ; ++w) {
                                    if (target.adjacent(c, w))
                                        target_paths2_bitgraph.add_edge(v, w);
                                }
                            }
                        }
                    }

                    for (int v = 0 ; v < target.size() ; ++v) {
                        for (int c = 0 ; c < target.size() ; ++c) {
                            if (target.adjacent(c, v)) {
                                for (int d = 0 ; d < target.size() ; ++d) {
                                    if (d != v && target.adjacent(c, d)) {
                                        for (int w = 0 ; w < v ; ++w) {
                                            if (w != c && target.adjacent(d, w))
                                                target_paths3_bitgraph.add_edge(v, w);
                                        }
                                    }
                                }
                            }
                        }
                    }

                }
            }

            if (expand(domains, -1, result.nodes)) {
                for (int i = 0 ; i < pattern.size() ; ++i)
                    result.isomorphism.emplace(i, domains.at(i).values.first_set_bit());
            }

            return result;
        }
    };

    template <template <unsigned, typename, bool, bool> class SGI_, bool b1_, bool b2_>
    struct Apply2Bool
    {
        template <unsigned size_, typename VertexType_> using Type = SGI_<size_, VertexType_, b1_, b2_>;
    };
}

auto parasols::b_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, false, false>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, false, false, false, false, false, false);
}

auto parasols::bnds_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, false, false>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, false, false, false, false, false);
}

auto parasols::bndsp3_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, true, false>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, false, false, false, false, false);
}

auto parasols::bndss_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, false, true>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, false, false, false, false, false);
}

auto parasols::bndsp3s_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, true, true>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, false, false, false, false, false);
}

auto parasols::bndsrp3s_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, true, true>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, true, false, false, false, false);
}

auto parasols::bndsrx2s_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, true, true>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, true, true, false, false, false);
}

auto parasols::bndsrx2lv_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, true, true>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, true, true, false, false, true);
}

auto parasols::bndsrx22s_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, true, true>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, true, true, true, false, false);
}

auto parasols::bndsrx22lv_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, true, true>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, true, true, true, true, false);
}

auto parasols::bndsrx3lv_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<Apply2Bool<SGI, true, true>::Type, SubgraphIsomorphismResult>(AllGraphSizes(), graphs.second, graphs.first, params, true, true, false, false, true, true);
}
