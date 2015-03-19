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
#include <future>

using namespace parasols;

using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::steady_clock;

namespace
{
    auto store_unless_already_less(std::atomic<int> & a, int v) -> void
    {
        while (true) {
            int c = a.load();
            if (v < c) {
                if (a.compare_exchange_strong(c, v))
                    break;
            }
            else
                break;
        }
    }

    struct Tasks
    {
        std::vector<std::thread> threads;
        std::atomic<bool> finish;
        std::atomic<int> busy;
        std::list<std::pair<std::function<void ()>, long long> > pending;
        long long task_number;
        std::mutex mutex;
        std::condition_variable cv;

        std::list<milliseconds> times;

        Tasks(unsigned n_threads)
        {
            finish = false;
            busy = 0;
            task_number = 0;

            for (unsigned t = 0 ; t < n_threads ; ++t)
                threads.emplace_back([this] {
                        milliseconds total_work_time = milliseconds::zero();

                        while (true) {
                            std::unique_lock<std::mutex> guard(mutex);
                            if (finish.load())
                                break;

                            if (! pending.empty()) {
                                auto f = std::move(pending.begin()->first);
                                pending.pop_front();
                                ++busy;
                                guard.unlock();

                                auto start_work_time = steady_clock::now(); // local start time

                                f();

                                auto work_time = duration_cast<milliseconds>(steady_clock::now() - start_work_time);
                                total_work_time += work_time;

                                guard.lock();
                                --busy;
                                cv.notify_all();
                            }
                            else
                                cv.wait(guard);
                        }

                        std::unique_lock<std::mutex> guard(mutex);
                        times.push_back(total_work_time);
                    });
        }

        auto run_first_if_equals(long long v) -> void
        {
            std::unique_lock<std::mutex> guard(mutex);

            if ((! pending.empty()) && pending.begin()->second == v) {
                auto f = std::move(pending.begin()->first);
                pending.pop_front();
                guard.unlock();

                f();

                guard.lock();
                cv.notify_all();
            }
        }

        auto kill_workers() -> void
        {
            {
                std::unique_lock<std::mutex> guard(mutex);
                finish = true;
                cv.notify_all();
            }

            for (auto & t : threads)
                t.join();

            threads.clear();
        }

        ~Tasks()
        {
            kill_workers();
        }

        auto add(std::function<void ()> && f) -> long long
        {
            std::unique_lock<std::mutex> guard(mutex);
            long long our_task_number = task_number++;
            pending.push_back(std::make_pair(std::move(f), our_task_number));
            cv.notify_all();
            return our_task_number;
        }

        auto complete() -> void
        {
            std::unique_lock<std::mutex> guard(mutex);
            while ((! pending.empty()) || (busy > 0))
                cv.wait(guard);
        }
    };

    enum class Search
    {
        Aborted,
        Unsatisfiable,
        Satisfiable
    };

    enum class AssignAndSearch
    {
        Aborted,
        Satisfiable,
        AssignFailed,
        Backjump,
        TryNext
    };

    template <unsigned n_words_, bool backjump_, int k_, int l_>
    struct TSGI :
        SupplementalGraphsMixin<TSGI<n_words_, backjump_, k_, l_>, n_words_, k_, l_>
    {
        using SupplementalGraphsMixin<TSGI<n_words_, backjump_, k_, l_>, n_words_, k_, l_>::build_supplemental_graphs;
        using SupplementalGraphsMixin<TSGI<n_words_, backjump_, k_, l_>, n_words_, k_, l_>::parallel_build_supplemental_graphs;

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
        using SubproblemResult = std::tuple<AssignAndSearch, FailedVariables, Assignments>;

        struct Subproblem
        {
            unsigned branch_v;
            int f_v;
        };

        const SubgraphIsomorphismParams & params;
        const bool parallel_for_loops;
        const int split_depth;

        static constexpr int max_graphs = 1 + (l_ - 1) * k_;
        std::array<FixedBitGraph<n_words_>, max_graphs> target_graphs;
        std::array<FixedBitGraph<n_words_>, max_graphs> pattern_graphs;

        std::vector<int> pattern_order, target_order, isolated_vertices;
        std::array<int, n_words_ * bits_per_word> pattern_degree_tiebreak;

        unsigned pattern_size, full_pattern_size, target_size;

        SubgraphIsomorphismResult result;

        Tasks tasks;

        TSGI(const Graph & target, const Graph & pattern, const SubgraphIsomorphismParams & a, bool tt, int sd) :
            params(a),
            parallel_for_loops(tt),
            split_depth(sd),
            target_order(target.size()),
            pattern_size(pattern.size()),
            full_pattern_size(pattern.size()),
            target_size(target.size()),
            tasks(params.n_threads - 1)
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
                    if (pattern.adjacent(pattern_order.at(i), pattern_order.at(j))) {
                        if ((! params.delete_loops) || (i != j))
                            pattern_graphs.at(0).add_edge(i, j);
                    }

            // determine ordering for target graph vertices
            std::iota(target_order.begin(), target_order.end(), 0);

            std::mt19937 g;
            std::shuffle(target_order.begin(), target_order.end(), g);
            degree_sort(target, target_order, false);

            // recode target to a bit graph
            target_graphs.at(0).resize(target_size);
            for (unsigned i = 0 ; i < target_size ; ++i)
                for (unsigned j = 0 ; j < target_size ; ++j)
                    if (target.adjacent(target_order.at(i), target_order.at(j))) {
                        if ((! params.delete_loops) || (i != j))
                            target_graphs.at(0).add_edge(i, j);
                    }

            for (unsigned j = 0 ; j < pattern_size ; ++j)
                pattern_degree_tiebreak.at(j) = pattern_graphs.at(0).degree(j);
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
                const int g_end,
                const int depth,
                const int cancel_after_pos,
                std::atomic<int> & cancel_after_val
                ) -> std::pair<Search, FailedVariables>
        {
            if (params.abort->load())
                return std::make_pair(Search::Aborted, FailedVariables());

            if (cancel_after_pos >= cancel_after_val.load())
                return std::make_pair(Search::Aborted, FailedVariables());

            ++nodes;

            Domain * branch_domain = nullptr;

            for (auto & d : domains)
                if ((! branch_domain) ||
                        d.popcount < branch_domain->popcount ||
                        (d.popcount == branch_domain->popcount && pattern_degree_tiebreak.at(d.v) > pattern_degree_tiebreak.at(branch_domain->v)))
                    branch_domain = &d;

            if (! branch_domain)
                return std::make_pair(Search::Satisfiable, FailedVariables());

            auto remaining = branch_domain->values;
            auto branch_v = branch_domain->v;

            FailedVariables shared_failed_variables;
            shared_failed_variables.add(branch_domain->v);

            if (depth > split_depth) {
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

                    auto search_result = search(assignments, new_domains, nodes, g_end, depth + 1, cancel_after_pos, cancel_after_val);
                    switch (search_result.first) {
                        case Search::Satisfiable:    return std::make_pair(Search::Satisfiable, FailedVariables());
                        case Search::Aborted:        return std::make_pair(Search::Aborted, FailedVariables());
                        case Search::Unsatisfiable:  break;
                    }

                    if (search_result.second.independent_of(domains, new_domains))
                        return search_result;

                    shared_failed_variables.add(search_result.second);
                }
            }
            else {
                std::vector<std::pair<std::packaged_task<SubproblemResult ()>, long long> > subproblem_tasks;
                subproblem_tasks.reserve(remaining.popcount());

                std::atomic<int> cancel_children_after; cancel_children_after = std::numeric_limits<int>::max();
                int cancel_n = 0;
                for (int f_v = remaining.first_set_bit() ; f_v != -1 ; f_v = remaining.first_set_bit()) {
                    remaining.unset(f_v);

                    subproblem_tasks.emplace_back(std::packaged_task<SubproblemResult ()>(
                            std::bind(uninlineable_parallel_search_haxx, this, branch_v, f_v, cancel_n, assignments, std::ref(cancel_children_after),
                                std::cref(domains), std::ref(nodes), g_end, depth)), -1);

                    ++cancel_n;
                }

                for (unsigned t = 1 ; t < subproblem_tasks.size() ; ++t) {
                    auto & s = subproblem_tasks.at(t).first;
                    subproblem_tasks.at(t).second = tasks.add([&s] { s(); });
                }

                int cancel_pos = 0;
                bool someone_aborted = false;
                for (auto & s : subproblem_tasks) {
                    if (&s == &*subproblem_tasks.begin())
                        s.first();
                    else
                        tasks.run_first_if_equals(s.second);

                    auto result = std::move(s.first.get_future().get());

                    switch (std::get<0>(result)) {
                        case AssignAndSearch::Satisfiable:
                            assignments = std::move(std::get<2>(result));
                            cancel_children_after.store(0);
                            tasks.complete();
                            return std::make_pair(Search::Satisfiable, std::get<1>(result));

                        case AssignAndSearch::Aborted:
                            someone_aborted = true;
                            continue;

                        case AssignAndSearch::Backjump:
                            cancel_children_after.store(0);
                            tasks.complete();
                            return std::make_pair(Search::Unsatisfiable, std::get<1>(result));

                        case AssignAndSearch::AssignFailed:
                            shared_failed_variables.add(std::get<1>(result));
                            break;

                        case AssignAndSearch::TryNext:
                            shared_failed_variables.add(std::get<1>(result));
                            break;
                    }

                    ++cancel_pos;
                }

                tasks.complete();

                if (someone_aborted)
                    return std::make_pair(Search::Aborted, FailedVariables());
            }

            return std::make_pair(Search::Unsatisfiable, shared_failed_variables);
        }

        auto parallel_subsearch(
                unsigned branch_v,
                int f_v,
                int cancel_n,
                Assignments local_assignments,
                std::atomic<int> & cancel_children_after,
                const Domains & domains,
                std::atomic<unsigned long long> & nodes,
                const int g_end,
                const int depth
                ) -> SubproblemResult
        {
            /* try assigning f_v to v */
            local_assignments.at(branch_v) = f_v;

            /* set up new domains */
            Domains new_domains;
            new_domains.reserve(domains.size() - 1);
            for (auto & d : domains)
                if (d.v != branch_v)
                    new_domains.push_back(d);

            /* assign and propagate */
            FailedVariables assign_failed_variables;
            if (! assign(new_domains, branch_v, f_v, g_end, assign_failed_variables))
                return std::make_tuple(AssignAndSearch::AssignFailed, assign_failed_variables, Assignments());

            auto search_result = search(local_assignments, new_domains, nodes, g_end, depth + 1, cancel_n, cancel_children_after);
            switch (search_result.first) {
                case Search::Satisfiable:
                    store_unless_already_less(cancel_children_after, 0);
                    return std::make_tuple(AssignAndSearch::Satisfiable, FailedVariables(), std::move(local_assignments));

                case Search::Aborted:
                    return std::make_tuple(AssignAndSearch::Aborted, FailedVariables(), Assignments());

                case Search::Unsatisfiable:
                    break;
            }

            if (search_result.second.independent_of(domains, new_domains)) {
                store_unless_already_less(cancel_children_after, cancel_n);
                return std::make_tuple(AssignAndSearch::Backjump, search_result.second, Assignments());
            }

            return std::make_tuple(AssignAndSearch::TryNext, search_result.second, Assignments());
        }

        static SubproblemResult uninlineable_parallel_search_haxx(
                TSGI * sgi,
                unsigned branch_v,
                int f_v,
                int cancel_n,
                Assignments local_assignments,
                std::atomic<int> & cancel_children_after,
                const Domains & domains,
                std::atomic<unsigned long long> & nodes,
                const int g_end,
                const int depth
                )
        {
            return std::move(sgi->parallel_subsearch(branch_v, f_v, cancel_n, local_assignments, cancel_children_after, domains, nodes, g_end, depth));
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

                if (parallel_for_loops) {
                    std::atomic<unsigned> posi1, posi2;
                    posi1 = 0, posi2 = 0;

                    for (unsigned t = 0 ; t < params.n_threads ; ++t) {
                        tasks.add([&] {
                            for (unsigned i ; ((i = posi1++)) < pattern_size ; ) {
                                for (int g = 0 ; g < g_end ; ++g) {
                                    for (unsigned j = 0 ; j < pattern_size ; ++j) {
                                        if (pattern_graphs.at(g).adjacent(i, j))
                                            patterns_ndss.at(g).at(i).push_back(patterns_degrees.at(g).at(j));
                                    }
                                    std::sort(patterns_ndss.at(g).at(i).begin(), patterns_ndss.at(g).at(i).end(), std::greater<int>());
                                }
                            }

                            for (unsigned i ; ((i = posi2++)) < target_size ; ) {
                                for (int g = 0 ; g < g_end ; ++g) {
                                    for (unsigned j = 0 ; j < target_size ; ++j) {
                                        if (target_graphs.at(g).adjacent(i, j))
                                            targets_ndss.at(g).at(i).push_back(targets_degrees.at(g).at(j));
                                    }
                                    std::sort(targets_ndss.at(g).at(i).begin(), targets_ndss.at(g).at(i).end(), std::greater<int>());
                                }
                            }
                        });
                    }

                    tasks.complete();

                    std::atomic<unsigned> posi;
                    posi = 0;

                    for (unsigned t = 0 ; t < params.n_threads ; ++t) {
                        tasks.add([&] {
                            for (unsigned i ; ((i = posi++)) < pattern_size ; ) {
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
                        });
                    }

                    tasks.complete();
                }
                else {
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

            if (parallel_for_loops)
                parallel_build_supplemental_graphs(params.n_threads);
            else
                build_supplemental_graphs();

            Domains domains(pattern_size);

            if (! initialise_domains(domains, max_graphs))
                return result;

            FailedVariables dummy_failed_variables;
            if (! cheap_all_different(domains, dummy_failed_variables))
                return result;

            prepare_for_search(domains);

            Assignments assignments(pattern_size, std::numeric_limits<unsigned>::max());
            std::atomic<unsigned long long> nodes{ 0 };
            std::atomic<int> cancel_children_after; cancel_children_after = std::numeric_limits<int>::max();
            switch (search(assignments, domains, nodes, max_graphs, 0, 0, cancel_children_after).first) {
                case Search::Satisfiable:
                    save_result(assignments, result);
                    break;

                case Search::Unsatisfiable:
                    break;

                case Search::Aborted:
                    break;
            }

            result.nodes = nodes;

            tasks.kill_workers();
            for (auto & t : tasks.times)
                result.times.push_back(t);

            return result;
        }
    };

    template <template <unsigned, bool, int, int> class SGI_, bool b_, int n_, int m_>
    struct Apply
    {
        template <unsigned size_, typename> using Type = SGI_<size_, b_, n_, m_>;
    };
}

auto parasols::ttvbbj_dpd_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    if (graphs.first.size() > graphs.second.size())
        return SubgraphIsomorphismResult{ };
    return select_graph_size<Apply<TSGI, true, 3, 3>::template Type, SubgraphIsomorphismResult>(
            AllGraphSizes(), graphs.second, graphs.first, params, true, 0);
}

auto parasols::tvbbj_dpd_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    if (graphs.first.size() > graphs.second.size())
        return SubgraphIsomorphismResult{ };
    return select_graph_size<Apply<TSGI, true, 3, 3>::template Type, SubgraphIsomorphismResult>(
            AllGraphSizes(), graphs.second, graphs.first, params, false, 0);
}

auto parasols::tvb_dpd_subgraph_isomorphism(const std::pair<Graph, Graph> & graphs, const SubgraphIsomorphismParams & params) -> SubgraphIsomorphismResult
{
    if (graphs.first.size() > graphs.second.size())
        return SubgraphIsomorphismResult{ };
    return select_graph_size<Apply<TSGI, false, 3, 3>::template Type, SubgraphIsomorphismResult>(
            AllGraphSizes(), graphs.second, graphs.first, params, false, 0);
}

