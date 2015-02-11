/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <subgraph_isomorphism/tvb_subgraph_isomorphism.hh>
#include <subgraph_isomorphism/supplemental_graphs.hh>

#include <graph/bit_graph.hh>
#include <graph/template_voodoo.hh>
#include <graph/degree_sort.hh>

#include <threads/queue.hh>

#include <algorithm>
#include <limits>
#include <random>

using namespace parasols;

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::steady_clock;

namespace
{
    enum class Search
    {
        Aborted,
        Unsatisfiable,
        Satisfiable
    };

    template <unsigned n_words_, bool backjump_, int k_, int l_>
    struct TSGI :
        SupplementalGraphsMixin<TSGI<n_words_, backjump_, k_, l_>, n_words_, k_, l_>
    {
        using SupplementalGraphsMixin<TSGI<n_words_, backjump_, k_, l_>, n_words_, k_, l_>::build_supplemental_graphs;

        struct Domain
        {
            unsigned v;
            unsigned popcount;
            FixedBitSet<n_words_> values;
        };

        using Domains = std::vector<Domain>;
        using Assignments = std::vector<unsigned>;

        struct DummyFailedVariables
        {
            auto independent_of(const Domains &, const Domains &) -> bool
            {
                return false;
            }

            auto add(unsigned) -> void
            {
            }

            auto add(const DummyFailedVariables &) -> void
            {
            }
        };

        struct RealFailedVariables
        {
            FixedBitSet<n_words_> variables;

            auto independent_of(const Domains & old_domains, const Domains & new_domains) -> bool
            {
                auto vc = variables;
                for (int v = vc.first_set_bit() ; v != -1 ; v = vc.first_set_bit()) {
                    vc.unset(v);

                    auto o = std::find_if(old_domains.begin(), old_domains.end(), [v] (const Domain & d) { return d.v == unsigned(v); });
                    auto n = std::find_if(new_domains.begin(), new_domains.end(), [v] (const Domain & d) { return d.v == unsigned(v); });

                    unsigned opc = (o == old_domains.end() ? 1 : o->popcount);
                    unsigned npc = (n == new_domains.end() ? 1 : n->popcount);
                    if (opc != npc)
                        return false;
                }

                return true;
            }

            auto add(unsigned dv) -> void
            {
                variables.set(dv);
            }

            auto add(const RealFailedVariables & d) -> void
            {
                variables.union_with(d.variables);
            }
        };

        using FailedVariables = typename std::conditional<backjump_, RealFailedVariables, DummyFailedVariables>::type;

        struct Subproblem
        {
            unsigned branch_v;
            int f_v;
        };

        const SubgraphIsomorphismParams & params;

        static constexpr int max_graphs = 1 + (l_ - 1) * k_;
        std::array<FixedBitGraph<n_words_>, max_graphs> target_graphs;
        std::array<FixedBitGraph<n_words_>, max_graphs> pattern_graphs;

        std::vector<int> pattern_order, target_order, isolated_vertices;
        std::array<int, n_words_ * bits_per_word> pattern_degree_tiebreak;

        unsigned pattern_size, full_pattern_size, target_size;

        std::atomic<bool> kill_everything;

        SubgraphIsomorphismResult result;

        TSGI(const Graph & target, const Graph & pattern, const SubgraphIsomorphismParams & a) :
            params(a),
            target_order(target.size()),
            pattern_size(pattern.size()),
            full_pattern_size(pattern.size()),
            target_size(target.size())
        {
            // strip out isolated vertices in the pattern
            for (unsigned v = 0 ; v < full_pattern_size ; ++v)
                if (0 == pattern.degree(v)) {
                    isolated_vertices.push_back(v);
                    --pattern_size;
                }
                else
                    pattern_order.push_back(v);

            // recode pattern to a bit graph
            pattern_graphs.at(0).resize(pattern_size);
            for (unsigned i = 0 ; i < pattern_size ; ++i)
                for (unsigned j = 0 ; j < pattern_size ; ++j)
                    if (pattern.adjacent(pattern_order.at(i), pattern_order.at(j)))
                        pattern_graphs.at(0).add_edge(i, j);

            // determine ordering for target graph vertices
            std::iota(target_order.begin(), target_order.end(), 0);

            std::mt19937 g;
            std::shuffle(target_order.begin(), target_order.end(), g);
            degree_sort(target, target_order, false);

            // recode target to a bit graph
            target_graphs.at(0).resize(target_size);
            for (unsigned i = 0 ; i < target_size ; ++i)
                for (unsigned j = 0 ; j < target_size ; ++j)
                    if (target.adjacent(target_order.at(i), target_order.at(j)))
                        target_graphs.at(0).add_edge(i, j);

            for (unsigned j = 0 ; j < pattern_size ; ++j)
                pattern_degree_tiebreak.at(j) = pattern_graphs.at(0).degree(j);

            kill_everything.store(false);
        }

        auto assign(Domains & new_domains, unsigned branch_v, unsigned f_v, int g_end, FailedVariables & failed_variables) -> bool
        {
            // for each remaining domain...
            for (auto & d : new_domains) {
                // all different
                d.values.unset(f_v);

                // for each graph pair...
                for (int g = 0 ; g < g_end ; ++g) {
                    // if we're adjacent...
                    if (pattern_graphs.at(g).adjacent(branch_v, d.v)) {
                        // ...then we can only be mapped to adjacent vertices
                        target_graphs.at(g).intersect_with_row(f_v, d.values);
                    }
                }

                // we might have removed values
                d.popcount = d.values.popcount();
                if (0 == d.popcount) {
                    failed_variables.add(d.v);
                    return false;
                }
            }

            FailedVariables all_different_failed_variables;
            if (! cheap_all_different(new_domains, all_different_failed_variables)) {
                failed_variables.add(all_different_failed_variables);
                return false;
            }

            return true;
        }

        auto search(
                Assignments & assignments,
                Domains & domains,
                std::atomic<unsigned long long> & nodes,
                int g_end,
                bool parallel) -> std::pair<Search, FailedVariables>
        {
            if (params.abort->load())
                return std::make_pair(Search::Aborted, FailedVariables());

            if (kill_everything.load())
                return std::make_pair(Search::Aborted, FailedVariables());

            ++nodes;

            Domain * branch_domain = nullptr;

            for (auto & d : domains)
                if ((! branch_domain) ||
                        d.popcount < branch_domain->popcount ||
                        (d.popcount == branch_domain->popcount && pattern_degree_tiebreak.at(d.v) > pattern_degree_tiebreak.at(branch_domain->v)))
                    branch_domain = &d;

            if (! branch_domain) {
                kill_everything.store(true);
                return std::make_pair(Search::Satisfiable, FailedVariables());
            }

            auto remaining = branch_domain->values;
            auto branch_v = branch_domain->v;

            FailedVariables shared_failed_variables;
            shared_failed_variables.add(branch_domain->v);

            if (! parallel) {
                for (int f_v = remaining.first_set_bit() ; f_v != -1 ; f_v = remaining.first_set_bit()) {
                    remaining.unset(f_v);

                    /* try assigning f_v to v */
                    assignments.at(branch_v) = f_v;

                    /* set up new domains */
                    Domains new_domains;
                    new_domains.reserve(domains.size() - 1);
                    for (auto & d : domains)
                        if (d.v != branch_v)
                            new_domains.push_back(d);

                    /* assign and propagate */
                    if (! assign(new_domains, branch_v, f_v, g_end, shared_failed_variables))
                        continue;

                    auto search_result = search(assignments, new_domains, nodes, g_end, false);
                    switch (search_result.first) {
                        case Search::Satisfiable:    return std::make_pair(Search::Satisfiable, FailedVariables());
                        case Search::Aborted:        return std::make_pair(Search::Aborted, FailedVariables());
                        case Search::Unsatisfiable:  break;
                    }

                    if (search_result.second.independent_of(domains, new_domains))
                        return search_result;

                    shared_failed_variables.add(search_result.second);
                }

                return std::make_pair(Search::Unsatisfiable, shared_failed_variables);
            }
            else {
                std::mutex shared_state_mutex;
                bool someone_succeeded = false;
                FailedVariables shared_failed_variables;
                shared_failed_variables.add(branch_domain->v);

                Queue<Subproblem> queue(params.n_threads, false);

                for (int f_v = remaining.first_set_bit() ; f_v != -1 ; f_v = remaining.first_set_bit()) {
                    remaining.unset(f_v);

                    queue.enqueue(Subproblem{ branch_v, f_v });
                }

                queue.initial_producer_done();

                std::list<std::thread> workers;

                for (unsigned t = 0 ; t < params.n_threads ; ++t) {
                    workers.emplace_back([&] {
                        auto start_time = steady_clock::now(); // local start time

                        Subproblem subproblem;
                        FailedVariables worker_shared_failed_variables;
                        bool stop = false;
                        while ((! stop) && queue.dequeue_blocking(subproblem)) {
                            /* set up new domains */
                            Domains new_domains;
                            new_domains.reserve(domains.size() - 1);
                            for (auto & d : domains)
                                if (d.v != subproblem.branch_v)
                                    new_domains.push_back(d);

                            Assignments subproblem_assignments = assignments;

                            /* try assigning f_v to v */
                            subproblem_assignments.at(subproblem.branch_v) = subproblem.f_v;
                            if (! assign(new_domains, subproblem.branch_v, subproblem.f_v, g_end, worker_shared_failed_variables))
                                continue;

                            auto search_result = search(subproblem_assignments, new_domains, nodes, g_end, false);
                            switch (search_result.first) {
                                case Search::Satisfiable:
                                    {
                                        std::unique_lock<std::mutex> guard(shared_state_mutex);
                                        assignments = subproblem_assignments;
                                        someone_succeeded = true;
                                    }
                                    stop = true;
                                    break;

                                case Search::Aborted:
                                    stop = true;
                                    break;

                                case Search::Unsatisfiable:
                                    break;
                            }

                            if (! stop) {
                                if (search_result.second.independent_of(domains, new_domains)) {
                                    kill_everything.store(true);
                                    return;
                                }
                            }
                        }

                        auto overall_time = duration_cast<milliseconds>(steady_clock::now() - start_time);

                        std::unique_lock<std::mutex> guard(shared_state_mutex);
                        shared_failed_variables.add(worker_shared_failed_variables);
                        result.times.push_back(overall_time);
                        });
                }

                for (auto & w : workers)
                    w.join();

                if (someone_succeeded)
                    return std::make_pair(Search::Satisfiable, shared_failed_variables);
                else
                    return std::make_pair(Search::Unsatisfiable, shared_failed_variables);
            }
        }

        auto initialise_domains(Domains & domains, int g_end) -> bool
        {
            unsigned remaining_target_vertices = target_size;
            FixedBitSet<n_words_> allowed_target_vertices;
            allowed_target_vertices.set_up_to(target_size);

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
                        FixedBitSet<n_words_> remaining = allowed_target_vertices;
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
                    domains.at(i).v = i;
                    domains.at(i).values.unset_all();

                    for (unsigned j = 0 ; j < target_size ; ++j) {
                        bool ok = true;

                        for (int g = 0 ; g < g_end ; ++g) {
                            if (! allowed_target_vertices.test(j)) {
                                ok = false;
                            }
                            else if (pattern_graphs.at(g).adjacent(i, i) && ! target_graphs.at(g).adjacent(j, j)) {
                                ok = false;
                            }
                            else if (targets_ndss.at(g).at(j).size() < patterns_ndss.at(g).at(i).size()) {
                                ok = false;
                            }
                            else {
                                for (unsigned x = 0 ; ok && x < patterns_ndss.at(g).at(i).size() ; ++x) {
                                    if (targets_ndss.at(g).at(j).at(x) < patterns_ndss.at(g).at(i).at(x))
                                        ok = false;
                                }
                            }

                            if (! ok)
                                break;
                        }

                        if (ok)
                            domains.at(i).values.set(j);
                    }

                    domains.at(i).popcount = domains.at(i).values.popcount();
                }

                FixedBitSet<n_words_> domains_union;
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

        auto cheap_all_different(Domains & domains, FailedVariables & failed_variables) -> bool
        {
            // pick domains smallest first, with tiebreaking
            std::array<int, n_words_ * bits_per_word> domains_order;
            std::iota(domains_order.begin(), domains_order.begin() + domains.size(), 0);

            std::sort(domains_order.begin(), domains_order.begin() + domains.size(),
                    [&] (int a, int b) {
                    return (domains.at(a).popcount < domains.at(b).popcount) ||
                    (domains.at(a).popcount == domains.at(b).popcount && pattern_degree_tiebreak.at(domains.at(a).v) > pattern_degree_tiebreak.at(domains.at(b).v));
                    });

            // counting all-different
            FixedBitSet<n_words_> domains_so_far, hall;
            unsigned neighbours_so_far = 0;

            for (int i = 0, i_end = domains.size() ; i != i_end ; ++i) {
                auto & d = domains.at(domains_order.at(i));

                failed_variables.add(d.v);

                d.values.intersect_with_complement(hall);
                d.popcount = d.values.popcount();

                if (0 == d.popcount)
                    return false;

                domains_so_far.union_with(d.values);
                ++neighbours_so_far;

                unsigned domains_so_far_popcount = domains_so_far.popcount();
                if (domains_so_far_popcount < neighbours_so_far) {
                    return false;
                }
                else if (domains_so_far_popcount == neighbours_so_far) {
                    neighbours_so_far = 0;
                    hall.union_with(domains_so_far);
                    domains_so_far.unset_all();
                }
            }

            return true;
        }

        auto prepare_for_search(Domains & domains) -> void
        {
            for (auto & d : domains)
                d.popcount = d.values.popcount();
        }

        auto save_result(Assignments & assignments, SubgraphIsomorphismResult & result) -> void
        {
            for (unsigned v = 0 ; v < pattern_size ; ++v)
                result.isomorphism.emplace(pattern_order.at(v), target_order.at(assignments.at(v)));

            int t = 0;
            for (auto & v : isolated_vertices) {
                while (result.isomorphism.end() != std::find_if(result.isomorphism.begin(), result.isomorphism.end(),
                            [&t] (const std::pair<int, int> & p) { return p.second == t; }))
                        ++t;
                result.isomorphism.emplace(v, t);
            }
        }

        auto run() -> SubgraphIsomorphismResult
        {
            if (full_pattern_size > target_size) {
                /* some of our fixed size data structures will throw a hissy
                 * fit. check this early. */
                return result;
            }

            build_supplemental_graphs();

            Domains domains(pattern_size);

            if (! initialise_domains(domains, max_graphs))
                return result;

            FailedVariables dummy_failed_variables;
            if (! cheap_all_different(domains, dummy_failed_variables))
                return result;

            prepare_for_search(domains);

            Assignments assignments(pattern_size, std::numeric_limits<unsigned>::max());
            std::atomic<unsigned long long> nodes;
            switch (search(assignments, domains, nodes, max_graphs, true).first) {
                case Search::Satisfiable:
                    save_result(assignments, result);
                    break;

                case Search::Unsatisfiable:
                    break;

                case Search::Aborted:
                    break;
            }

            result.nodes = nodes;

            return result;
        }
    };

    template <template <unsigned, bool, int, int> class SGI_, bool b_, int n_, int m_>
    struct Apply
    {
        template <unsigned size_, typename> using Type = SGI_<size_, b_, n_, m_>;
    };
}

auto parasols::tvbbj_dpd_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    if (graphs.first.size() > graphs.second.size())
        return SubgraphIsomorphismResult{ };
    return select_graph_size<Apply<TSGI, true, 3, 3>::template Type, SubgraphIsomorphismResult>(
            AllGraphSizes(), graphs.second, graphs.first, params);
}

