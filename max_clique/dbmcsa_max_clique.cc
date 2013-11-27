/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/dbmcsa_max_clique.hh>
#include <max_clique/colourise.hh>
#include <max_clique/print_incumbent.hh>
#include <graph/degree_sort.hh>
#include <graph/min_width_sort.hh>
#include <threads/atomic_incumbent.hh>

#include <algorithm>
#include <thread>
#include <condition_variable>
#include <mutex>

using namespace parasols;

namespace
{
    template <unsigned size_>
    struct Job
    {
        std::mutex mutex;
        std::array<unsigned, size_ * bits_per_word> p_order;
        std::array<unsigned, size_ * bits_per_word> colours;
        FixedBitSet<size_> c;
        FixedBitSet<size_> p;
    };

    template <unsigned size_>
    auto found_possible_new_best(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,
            const FixedBitSet<size_> & c,
            int c_popcount,
            const MaxCliqueParams & params,
            MaxCliqueResult & result,
            AtomicIncumbent & best_anywhere) -> void
    {
        if (best_anywhere.update(c_popcount)) {
            result.size = c_popcount;
            result.members.clear();
            for (int i = 0 ; i < graph.size() ; ++i)
                if (c.test(i))
                    result.members.insert(o[i]);
            print_incumbent(params, result.size);
        }
    }

    auto bound(unsigned c_popcount, unsigned cn, const MaxCliqueParams & params, AtomicIncumbent & best_anywhere) -> bool
    {
        unsigned best_anywhere_value = best_anywhere.get();
        return (c_popcount + cn <= best_anywhere_value || best_anywhere_value >= params.stop_after_finding);
    }

    template <unsigned size_>
    auto expand_unstealable(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,                      // vertex ordering
            const std::array<unsigned, size_ * bits_per_word> & p_order,
            const std::array<unsigned, size_ * bits_per_word> & colours,
            FixedBitSet<size_> & c,                          // current candidate clique
            FixedBitSet<size_> & p,                          // potential additions
            MaxCliqueResult & result,
            AtomicIncumbent & best_anywhere,
            const MaxCliqueParams & params
            ) -> void
    {
        auto c_popcount = c.popcount();
        int n = p.popcount() - 1;

        // bound, timeout or early exit?
        if (bound(c_popcount, colours[n], params, best_anywhere) || params.abort.load())
            return;

        auto v = p_order[n];

        // consider taking v
        c.set(v);
        ++c_popcount;

        // filter p to contain vertices adjacent to v
        FixedBitSet<size_> new_p = p;
        graph.intersect_with_row(v, new_p);

        if (new_p.empty()) {
            found_possible_new_best(graph, o, c, c_popcount, params, result, best_anywhere);
        }
        else {
            // get our coloured vertices
            ++result.nodes;
            std::array<unsigned, size_ * bits_per_word> new_p_order, new_colours;
            colourise<size_>(graph, new_p, new_p_order, new_colours);
            expand_unstealable<size_>(graph, o, new_p_order, new_colours, c, new_p, result, best_anywhere, params);
        }

        // now consider not taking v
        c.unset(v);
        p.unset(v);
        if (n > 0) {
            expand_unstealable<size_>(graph, o, p_order, colours, c, p, result, best_anywhere, params);
        }
    }

    template <unsigned size_>
    auto expand(
            const FixedBitGraph<size_> & graph,
            const std::vector<int> & o,
            Job<size_> * job,
            std::unique_lock<std::mutex> & job_guard,
            Job<size_> * child_job,
            std::mutex & stealing_mutex,
            std::condition_variable & stealing_cv,
            std::list<Job<size_> *> & steal_queue,
            MaxCliqueResult & result,
            AtomicIncumbent & best_anywhere,
            const MaxCliqueParams & params
            ) -> bool
    {
        auto job_c_popcount = job->c.popcount();
        int n = job->p.popcount() - 1;

        // bound, timeout or early exit?
        if (-1 == n || bound(job_c_popcount, job->colours[n], params, best_anywhere) || params.abort.load())
            return false;

        auto v = job->p_order[n];

        auto c = job->c;
        auto p = job->p;

        // prepare next step in the job
        job->c.unset(v);
        job->p.unset(v);
        job_guard.unlock();

        // consider taking v
        c.set(v);

        // filter p to contain vertices adjacent to v
        FixedBitSet<size_> new_p = p;
        graph.intersect_with_row(v, new_p);

        if (new_p.empty()) {
            found_possible_new_best(graph, o, c, c.popcount(), params, result, best_anywhere);
        }
        else {
            // get our coloured vertices
            ++result.nodes;

            if (child_job) {
                child_job->c = c;
                child_job->p = new_p;
                colourise<size_>(graph, child_job->p, child_job->p_order, child_job->colours);

                {
                    std::unique_lock<std::mutex> guard(stealing_mutex);
                    steal_queue.push_back(child_job);
                    stealing_cv.notify_all();
                }

                while (true) {
                    std::unique_lock<std::mutex> child_job_guard(child_job->mutex);
                    if (! expand<size_>(graph, o, child_job, child_job_guard, nullptr,
                                stealing_mutex, stealing_cv, steal_queue, result, best_anywhere, params))
                        break;
                }

                {
                    std::unique_lock<std::mutex> guard(stealing_mutex);
                    steal_queue.remove(child_job);
                }
            }
            else {
                std::array<unsigned, size_ * bits_per_word> new_p_order, new_colours;
                colourise<size_>(graph, new_p, new_p_order, new_colours);
                expand_unstealable<size_>(graph, o, new_p_order, new_colours, c, new_p, result, best_anywhere, params);
            }
        }

        return true;
    }


    template <MaxCliqueOrder order_, unsigned size_>
    auto dbmcsa(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
    {
        MaxCliqueResult result;
        std::mutex result_mutex;
        result.size = params.initial_bound;

        AtomicIncumbent best_anywhere; // global incumbent
        best_anywhere.update(params.initial_bound);

        std::vector<int> o(graph.size()); // vertex ordering

        // populate our order with every vertex initially
        std::iota(o.begin(), o.end(), 0);

        switch (order_) {
            case MaxCliqueOrder::Degree:
                degree_sort(graph, o, false);
                break;
            case MaxCliqueOrder::MinWidth:
                min_width_sort(graph, o, false);
                break;
            case MaxCliqueOrder::ExDegree:
                exdegree_sort(graph, o, false);
                break;
            case MaxCliqueOrder::DynExDegree:
                dynexdegree_sort(graph, o, false);
                break;
        }

        // re-encode graph as a bit graph
        FixedBitGraph<size_> bit_graph;
        bit_graph.resize(graph.size());

        for (int i = 0 ; i < graph.size() ; ++i)
            for (int j = 0 ; j < graph.size() ; ++j)
                if (graph.adjacent(o[i], o[j]))
                    bit_graph.add_edge(i, j);

        // initial ordering
        ++result.nodes;

        Job<size_> top_job;

        // first job
        top_job.c.resize(graph.size());
        top_job.p.resize(graph.size());
        top_job.p.set_all();
        colourise<size_>(bit_graph, top_job.p, top_job.p_order, top_job.colours);

        std::mutex stealing_mutex;
        std::condition_variable stealing_cv;
        bool finished = false;
        int number_busy = params.n_threads;
        std::list<Job<size_> *> steal_queue;

        std::list<std::thread> threads; // workers
        for (unsigned i = 0 ; i < params.n_threads ; ++i) {
            threads.push_back(std::thread([&, i] {

                auto start_time = std::chrono::steady_clock::now(); // local start time
                auto last_useful_time = start_time; // gets updated
                MaxCliqueResult tr; // local result

                Job<size_> this_thread_job;

                while (true) {
                    std::unique_lock<std::mutex> guard(top_job.mutex);
                    if (! expand<size_>(bit_graph, o,
                            &top_job, guard, &this_thread_job,
                            stealing_mutex, stealing_cv, steal_queue,
                            tr, best_anywhere, params))
                        break;
                }

                last_useful_time = std::chrono::steady_clock::now();

                while (true) {
                    std::unique_lock<std::mutex> guard(stealing_mutex);
                    if (finished)
                        break;

                    if (0 == --number_busy) {
                        finished = true;
                        stealing_cv.notify_all();
                    } else {
                        bool found_something = false;
                        for (auto s = steal_queue.begin() ; s != steal_queue.end() ; ++s) {
                            std::unique_lock<std::mutex> steal_guard((*s)->mutex);
                            if (! (*s)->p.empty()) {
                                found_something = true;
                                ++number_busy;
                                bool resplit = ((*s)->c.popcount() <= 1);
                                guard.unlock();
                                expand<size_>(bit_graph, o, *s, steal_guard, resplit ? &this_thread_job : nullptr,
                                        stealing_mutex, stealing_cv, steal_queue, tr, best_anywhere, params);
                                last_useful_time = std::chrono::steady_clock::now();
                                break;
                            }
                        }

                        if (! found_something)
                            stealing_cv.wait(guard);
                    }
                }

                auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(last_useful_time - start_time);

                // merge results
                {
                    std::unique_lock<std::mutex> guard(result_mutex);
                    result.merge(tr);
                    result.times.push_back(overall_time);
                }
            }));
        }

        for (auto & t : threads)
            t.join();

        return result;
    }
}

template <MaxCliqueOrder order_>
auto parasols::dbmcsa_max_clique(const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult
{
    /* This is pretty horrible: in order to avoid dynamic allocation, select
     * the appropriate specialisation for our graph's size. */
    static_assert(max_graph_words == 1024, "Need to update here if max_graph_size is changed.");
    if (graph.size() < bits_per_word)
        return dbmcsa<order_, 1>(graph, params);
    else if (graph.size() < 2 * bits_per_word)
        return dbmcsa<order_, 2>(graph, params);
    else if (graph.size() < 4 * bits_per_word)
        return dbmcsa<order_, 4>(graph, params);
    else if (graph.size() < 8 * bits_per_word)
        return dbmcsa<order_, 8>(graph, params);
    else if (graph.size() < 16 * bits_per_word)
        return dbmcsa<order_, 16>(graph, params);
    else if (graph.size() < 32 * bits_per_word)
        return dbmcsa<order_, 32>(graph, params);
    else if (graph.size() < 64 * bits_per_word)
        return dbmcsa<order_, 64>(graph, params);
    else if (graph.size() < 128 * bits_per_word)
        return dbmcsa<order_, 128>(graph, params);
    else if (graph.size() < 256 * bits_per_word)
        return dbmcsa<order_, 256>(graph, params);
    else if (graph.size() < 512 * bits_per_word)
        return dbmcsa<order_, 512>(graph, params);
    else if (graph.size() < 1024 * bits_per_word)
        return dbmcsa<order_, 1024>(graph, params);
    else
        throw GraphTooBig();
}

template auto parasols::dbmcsa_max_clique<MaxCliqueOrder::Degree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::dbmcsa_max_clique<MaxCliqueOrder::MinWidth>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::dbmcsa_max_clique<MaxCliqueOrder::ExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;
template auto parasols::dbmcsa_max_clique<MaxCliqueOrder::DynExDegree>(const Graph &, const MaxCliqueParams &) -> MaxCliqueResult;

