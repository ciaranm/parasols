/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <subgraph_isomorphism/mb_subgraph_isomorphism.hh>

#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>
#include <graph/degree_sort.hh>

#include <algorithm>
#include <limits>
#include <chrono>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/max_cardinality_matching.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/strong_components.hpp>

using namespace parasols;

using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::duration_cast;

namespace
{
    template <unsigned target_size_>
    struct Domain
    {
        bool uncommitted_singleton = false;
        bool committed = false;
        bool on_edge_stack = false;
        unsigned popcount;

        FixedBitSet<target_size_> values;
    };

    template <unsigned target_size_, typename>
    struct MBSGI
    {
        using Domains = std::vector<Domain<target_size_> >;

        const SubgraphIsomorphismParams & params;

        static constexpr int max_graphs = 5;
        std::array<FixedBitGraph<target_size_>, max_graphs> target_graphs;
        std::array<FixedBitGraph<target_size_>, max_graphs> pattern_graphs;

        std::vector<int> order;

        unsigned pattern_size, target_size;

        MBSGI(const Graph & target, const Graph & pattern, const SubgraphIsomorphismParams & a) :
            params(a),
            order(target.size()),
            pattern_size(pattern.size()),
            target_size(target.size())
        {
            std::iota(order.begin(), order.end(), 0);
            degree_sort(target, order, false);

            pattern_graphs.at(0).resize(pattern_size);
            for (unsigned i = 0 ; i < pattern_size ; ++i)
                for (unsigned j = 0 ; j < pattern_size ; ++j)
                    if (pattern.adjacent(i, j))
                        pattern_graphs.at(0).add_edge(i, j);

            target_graphs.at(0).resize(target_size);
            for (unsigned i = 0 ; i < target_size ; ++i)
                for (unsigned j = 0 ; j < target_size ; ++j)
                    if (target.adjacent(order.at(i), order.at(j)))
                        target_graphs.at(0).add_edge(i, j);
        }

        auto filter(Domains & domains, int branch_point, int g_end) -> bool
        {
            /* really a vector, but memory allocation is expensive. only needs
             * room for pattern_size elements, but target_size_ will do. */
            std::array<unsigned, target_size_ * bits_per_word> uncommitted_singletons;
            int uncommitted_singletons_end = 0;

            if (-1 == branch_point) {
                /* no branch point at the top of search, check all domains */
                for (unsigned v = 0 ; v < pattern_size ; ++v) {
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
                unsigned v = uncommitted_singletons[--uncommitted_singletons_end];
                domains.at(v).uncommitted_singleton = false;
                domains.at(v).committed = true;

                /* what's it mapped to? */
                int f_v = domains.at(v).values.first_set_bit();

                /* how many unassigned neighbours do we have, and what is their domain? */
                std::array<unsigned, max_graphs> unassigned_neighbours;
                std::fill(unassigned_neighbours.begin(), unassigned_neighbours.end(), 0);

                std::array<FixedBitSet<target_size_>, max_graphs> unassigned_neighbours_domains_union;
                for (int g = 0 ; g < g_end ; ++g)
                    unassigned_neighbours_domains_union.at(g).resize(target_size);

                /* for each other domain... */
                for (unsigned w = 0 ; w < pattern_size ; ++w) {
                    if (v == w || domains.at(w).committed)
                        continue;

                    bool w_domain_potentially_changed = false;

                    if (domains.at(w).values.test(f_v)) {
                        /* no other domain can be mapped to f_v */
                        domains.at(w).values.unset(f_v);
                        w_domain_potentially_changed = true;
                    }

                    for (int g = 0 ; g < g_end ; ++g) {
                        if (pattern_graphs.at(g).adjacent(v, w)) {
                            /* v is adjacent to w, so w can only be mapped to things adjacent to f_v */
                            target_graphs.at(g).intersect_with_row(f_v, domains.at(w).values);
                            w_domain_potentially_changed = true;

                            ++unassigned_neighbours.at(g);
                            unassigned_neighbours_domains_union.at(g).union_with(domains.at(w).values);
                        }
                        else if (0 == g && params.induced) {
                            /* v is nonadjacent to w, so w can only be mapped to things nonadjacent to f_v */
                            target_graphs.at(0).intersect_with_row_complement(f_v, domains.at(w).values);
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
                for (int g = 0 ; g < g_end ; ++g) {
                    if (0 != unassigned_neighbours.at(g)) {
                        if (unassigned_neighbours.at(g) > unassigned_neighbours_domains_union.at(g).popcount())
                            return false;
                    }
                }
            }

            FixedBitSet<target_size_> domains_union;
            domains_union.resize(pattern_size);
            for (auto & d : domains)
                domains_union.union_with(d.values);

            if (unsigned(domains_union.popcount()) < pattern_size)
                return false;

            return true;
        }

        auto expand(Domains & domains, int branch_point, unsigned long long & nodes, int g_end, unsigned long long nodes_limit) -> bool
        {
            ++nodes;

            if (0 != nodes_limit && nodes > nodes_limit)
                return false;

            if (params.abort->load())
                return false;

            if (! filter(domains, branch_point, g_end))
                return false;

            int branch_on = -1;
            unsigned branch_on_popcount = 0;
            for (unsigned i = 0 ; i < pattern_size ; ++i) {
                if (! domains.at(i).committed) {
                    unsigned popcount = domains.at(i).popcount;
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
                new_domains.at(branch_on).values.unset_all();
                new_domains.at(branch_on).values.set(v);
                new_domains.at(branch_on).uncommitted_singleton = true;

                if (expand(new_domains, branch_on, nodes, g_end, nodes_limit)) {
                    domains = std::move(new_domains);
                    return true;
                }
            }

            return false;
        }

        auto regin_all_different(Domains & domains) -> bool
        {
            boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> match(pattern_size + target_size);

            unsigned consider = 0;
            for (unsigned i = 0 ; i < pattern_size ; ++i) {
                if (domains.at(i).values.popcount() < pattern_size)
                    ++consider;

                for (unsigned j = 0 ; j < target_size ; ++j) {
                    if (domains.at(i).values.test(j))
                        boost::add_edge(i, pattern_size + j, match);
                }
            }

            if (0 == consider)
                return true;

            std::vector<boost::graph_traits<decltype(match)>::vertex_descriptor> mate(pattern_size + target_size);
            boost::edmonds_maximum_cardinality_matching(match, &mate.at(0));

            std::set<int> free;
            for (unsigned j = 0 ; j < target_size ; ++j)
                free.insert(pattern_size + j);

            unsigned count = 0;
            for (unsigned i = 0 ; i < pattern_size ; ++i)
                if (mate.at(i) != boost::graph_traits<decltype(match)>::null_vertex()) {
                    ++count;
                    free.erase(mate.at(i));
                }

            if (count != unsigned(pattern_size))
                return false;

            boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS> match_o(pattern_size + target_size);
            std::set<std::pair<unsigned, unsigned> > unused;
            for (unsigned i = 0 ; i < pattern_size ; ++i) {
                for (unsigned j = 0 ; j < target_size ; ++j) {
                    if (domains.at(i).values.test(j)) {
                        unused.emplace(i, j);
                        if (mate.at(i) == unsigned(j + pattern_size))
                            boost::add_edge(i, pattern_size + j, match_o);
                        else
                            boost::add_edge(pattern_size + j, i, match_o);
                    }
                }
            }

            std::set<int> pending = free, seen;
            while (! pending.empty()) {
                unsigned v = *pending.begin();
                pending.erase(pending.begin());
                if (! seen.count(v)) {
                    seen.insert(v);
                    auto w = boost::adjacent_vertices(v, match_o);
                    for ( ; w.first != w.second ; ++w.first) {
                        if (*w.first >= unsigned(pattern_size))
                            unused.erase(std::make_pair(v, *w.first - pattern_size));
                        else
                            unused.erase(std::make_pair(*w.first, v - pattern_size));
                        pending.insert(*w.first);
                    }
                }
            }

            std::vector<int> component(num_vertices(match_o)), discover_time(num_vertices(match_o));
            std::vector<boost::default_color_type> color(num_vertices(match_o));
            std::vector<boost::graph_traits<decltype(match_o)>::vertex_descriptor> root(num_vertices(match_o));
            boost::strong_components(match_o,
                    make_iterator_property_map(component.begin(), get(boost::vertex_index, match_o)),
                    root_map(make_iterator_property_map(root.begin(), get(boost::vertex_index, match_o))).
                    color_map(make_iterator_property_map(color.begin(), get(boost::vertex_index, match_o))).
                    discover_time_map(make_iterator_property_map(discover_time.begin(), get(boost::vertex_index, match_o))));

            for (auto e = unused.begin(), e_end = unused.end() ; e != e_end ; ) {
                if (component.at(e->first) == component.at(e->second + pattern_size))
                    unused.erase(e++);
                else
                    ++e;
            }

            unsigned deletions = 0;
            for (auto & u : unused)
                if (mate.at(u.first) != u.second + pattern_size) {
                    ++deletions;
                    domains.at(u.first).values.unset(u.second);
                }

            return true;
        }

        auto nds_filter(Domains & domains, int g_end) -> bool
        {
            unsigned remaining_target_vertices = target_size;
            FixedBitSet<target_size_> allowed_target_vertices;
            allowed_target_vertices.resize(target_size);
            allowed_target_vertices.set_all();

            while (true) {
                std::array<std::vector<int>, max_graphs> patterns_degrees;
                std::array<std::vector<int>, max_graphs> targets_degrees;

                for (int g = 0 ; g < g_end ; ++g) {
                    patterns_degrees.at(g).resize(pattern_size);
                    targets_degrees.at(g).resize(target_size);
                }

                /* pattern and target degree sequences */
                for (int g = 0 ; g < g_end ; ++g) {
                    for (unsigned i = 0 ; i < pattern_size ; ++i)
                        patterns_degrees.at(g).at(i) = pattern_graphs.at(g).degree(i);

                    for (unsigned i = 0 ; i < target_size ; ++i) {
                        FixedBitSet<target_size_> remaining = allowed_target_vertices;
                        target_graphs.at(g).intersect_with_row(i, remaining);
                        targets_degrees.at(g).at(i) = remaining.popcount();
                    }
                }

                /* pattern and target neighbourhood degree sequences */
                std::array<std::vector<std::vector<int> >, max_graphs> patterns_ndss;
                std::array<std::vector<std::vector<int> >, max_graphs> targets_ndss;

                for (int g = 0 ; g < g_end ; ++g) {
                    patterns_ndss.at(g).resize(pattern_size);
                    targets_ndss.at(g).resize(target_size);
                }

                for (int g = 0 ; g < g_end ; ++g) {
                    for (unsigned i = 0 ; i < pattern_size ; ++i) {
                        for (unsigned j = 0 ; j < pattern_size ; ++j) {
                            if (pattern_graphs.at(g).adjacent(i, j))
                                patterns_ndss.at(g).at(i).push_back(patterns_degrees.at(g).at(j));
                        }
                        std::sort(patterns_ndss.at(g).at(i).begin(), patterns_ndss.at(g).at(i).end(), std::greater<int>());
                    }

                    for (unsigned i = 0 ; i < target_size ; ++i) {
                        for (unsigned j = 0 ; j < target_size ; ++j) {
                            if (target_graphs.at(g).adjacent(i, j))
                                targets_ndss.at(g).at(i).push_back(targets_degrees.at(g).at(j));
                        }
                        std::sort(targets_ndss.at(g).at(i).begin(), targets_ndss.at(g).at(i).end(), std::greater<int>());
                    }
                }

                for (unsigned i = 0 ; i < pattern_size ; ++i) {
                    domains.at(i).values.unset_all();
                    domains.at(i).values.resize(target_size);

                    for (unsigned j = 0 ; j < target_size ; ++j) {
                        bool ok = true;

                        for (int g = 0 ; g < g_end ; ++g) {
                            if (! allowed_target_vertices.test(j)) {
                                ok = false;
                            }
                            else if (pattern_graphs.at(g).adjacent(i, i) && ! target_graphs.at(g).adjacent(j, j)) {
                                ok = false;
                            }
                            else if (params.induced && target_graphs.at(g).adjacent(j, j) && ! pattern_graphs.at(g).adjacent(i, i)) {
                                ok = false;
                            }
                            else if (targets_ndss.at(g).at(j).size() >= patterns_ndss.at(g).at(i).size()) {
                                for (unsigned x = 0 ; x < patterns_ndss.at(g).at(i).size() ; ++x) {
                                    if (targets_ndss.at(g).at(j).at(x) < patterns_ndss.at(g).at(i).at(x)) {
                                        ok = false;
                                        break;
                                    }
                                }
                            }
                            else
                                ok = false;

                            if (! ok)
                                break;
                        }

                        if (ok)
                            domains.at(i).values.set(j);
                    }
                }

                FixedBitSet<target_size_> domains_union;
                domains_union.resize(pattern_size);
                for (auto & d : domains)
                    domains_union.union_with(d.values);

                unsigned domains_union_popcount = domains_union.popcount();
                if (domains_union_popcount < unsigned(pattern_size)) {
                    return false;
                }
                else if (domains_union_popcount == remaining_target_vertices)
                    break;

                allowed_target_vertices.intersect_with(domains_union);
                remaining_target_vertices = allowed_target_vertices.popcount();
            }

            return true;
        }

        auto build_path_graphs() -> void
        {
            pattern_graphs.at(1).resize(pattern_size);
            pattern_graphs.at(2).resize(pattern_size);
            pattern_graphs.at(3).resize(pattern_size);
            pattern_graphs.at(4).resize(pattern_size);

            for (unsigned v = 0 ; v < pattern_size ; ++v)
                for (unsigned c = 0 ; c < pattern_size ; ++c)
                    if (pattern_graphs.at(0).adjacent(c, v))
                        for (unsigned w = 0 ; w < v ; ++w)
                            if (pattern_graphs.at(0).adjacent(c, w)) {
                                if (pattern_graphs.at(1).adjacent(v, w))
                                    pattern_graphs.at(2).add_edge(v, w);
                                else
                                    pattern_graphs.at(1).add_edge(v, w);
                            }

            for (unsigned v = 0 ; v < pattern_size ; ++v) {
                for (unsigned c = 0 ; c < pattern_size ; ++c) {
                    if (pattern_graphs.at(0).adjacent(c, v)) {
                        for (unsigned d = 0 ; d < pattern_size ; ++d) {
                            if (d != v && pattern_graphs.at(0).adjacent(c, d)) {
                                for (unsigned w = 0 ; w < v ; ++w) {
                                    if (w != c && pattern_graphs.at(0).adjacent(d, w)) {
                                        if (pattern_graphs.at(3).adjacent(v, w))
                                            pattern_graphs.at(4).add_edge(v, w);
                                        else
                                            pattern_graphs.at(3).add_edge(v, w);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            target_graphs.at(1).resize(target_size);
            target_graphs.at(2).resize(target_size);
            target_graphs.at(3).resize(target_size);
            target_graphs.at(4).resize(target_size);

            for (unsigned v = 0 ; v < target_size ; ++v) {
                for (unsigned c = 0 ; c < target_size ; ++c) {
                    if (target_graphs.at(0).adjacent(c, v)) {
                        for (unsigned w = 0 ; w < v ; ++w) {
                            if (target_graphs.at(0).adjacent(c, w)) {
                                if (target_graphs.at(1).adjacent(v, w))
                                    target_graphs.at(2).add_edge(v, w);
                                else
                                    target_graphs.at(1).add_edge(v, w);
                            }
                        }
                    }
                }
            }

            for (unsigned v = 0 ; v < target_size ; ++v) {
                for (unsigned c = 0 ; c < target_size ; ++c) {
                    if (target_graphs.at(0).adjacent(c, v)) {
                        for (unsigned d = 0 ; d < target_size ; ++d) {
                            if (d != v && target_graphs.at(0).adjacent(c, d)) {
                                for (unsigned w = 0 ; w < v ; ++w) {
                                    if (w != c && target_graphs.at(0).adjacent(d, w)) {
                                        if (target_graphs.at(3).adjacent(v, w))
                                            target_graphs.at(4).add_edge(v, w);
                                        else
                                            target_graphs.at(3).add_edge(v, w);
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }

        auto run() -> SubgraphIsomorphismResult
        {
            SubgraphIsomorphismResult result;

            if (pattern_size > target_size) {
                /* some of our fixed size data structures will throw a hissy
                 * fit. check this early. */
                return result;
            }

            Domains domains(pattern_size);

            if (! nds_filter(domains, 1))
                return result;

            {
                auto domains_copy = domains;
                if (expand(domains_copy, -1, result.nodes, 1, pattern_size * pattern_size)) {
                    for (unsigned i = 0 ; i < pattern_size ; ++i)
                        result.isomorphism.emplace(i, order.at(domains_copy.at(i).values.first_set_bit()));
                    return result;
                }
            }

            build_path_graphs();

            if (! nds_filter(domains, max_graphs))
                return result;

            if (! regin_all_different(domains))
                return result;

            if (expand(domains, -1, result.nodes, max_graphs, 0)) {
                for (unsigned i = 0 ; i < pattern_size ; ++i)
                    result.isomorphism.emplace(i, order.at(domains.at(i).values.first_set_bit()));
            }

            return result;
        }
    };
}

auto parasols::mb_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    return select_graph_size<MBSGI, SubgraphIsomorphismResult>(
            AllGraphSizes(), graphs.second, graphs.first, params);
}

