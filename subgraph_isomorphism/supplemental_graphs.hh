/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_SUPPLEMENTAL_GRAPHS_HH
#define PARASOLS_GUARD_SUBGRAPH_ISOMORPHISM_SUPPLEMENTAL_GRAPHS_HH 1

#include <thread>
#include <vector>
#include <atomic>

namespace parasols
{
    template <class Parent_, unsigned n_words_, int k_, int l_>
    struct SupplementalGraphsMixin
    {
        auto build_supplemental_graphs() -> void
        {
            auto & max_graphs = static_cast<Parent_ *>(this)->max_graphs;
            auto & pattern_size = static_cast<Parent_ *>(this)->pattern_size;
            auto & target_size = static_cast<Parent_ *>(this)->target_size;
            auto & pattern_graphs = static_cast<Parent_ *>(this)->pattern_graphs;
            auto & target_graphs = static_cast<Parent_ *>(this)->target_graphs;

            for (int g = 1 ; g < max_graphs ; ++g)
                pattern_graphs.at(g).resize(pattern_size);

            if (l_ >= 2) {
                for (unsigned v = 0 ; v < pattern_size ; ++v) {
                    auto nv = pattern_graphs.at(0).neighbourhood(v);
                    for (int c = nv.first_set_bit() ; c != -1 ; c = nv.first_set_bit()) {
                        nv.unset(c);
                        auto nc = pattern_graphs.at(0).neighbourhood(c);
                        for (int w = nc.first_set_bit() ; w != -1 && unsigned(w) <= v ; w = nc.first_set_bit()) {
                            nc.unset(w);
                            if (k_ >= 3 && pattern_graphs.at(2).adjacent(v, w))
                                pattern_graphs.at(3).add_edge(v, w);
                            else if (k_ >= 2 && pattern_graphs.at(1).adjacent(v, w))
                                pattern_graphs.at(2).add_edge(v, w);
                            else if (k_ >= 1)
                                pattern_graphs.at(1).add_edge(v, w);
                        }
                    }
                }
            }

            if (l_ >= 3) {
                for (unsigned v = 0 ; v < pattern_size ; ++v) {
                    auto nv = pattern_graphs.at(0).neighbourhood(v);
                    for (int c = nv.first_set_bit() ; c != -1 ; c = nv.first_set_bit()) {
                        nv.unset(c);
                        auto nc = pattern_graphs.at(0).neighbourhood(c);
                        for (int d = nc.first_set_bit() ; d != -1 ; d = nc.first_set_bit()) {
                            nc.unset(d);
                            if (unsigned(d) == v)
                                continue;

                            auto nd = pattern_graphs.at(0).neighbourhood(d);
                            for (int w = nd.first_set_bit() ; w != -1 && unsigned(w) <= v ; w = nd.first_set_bit()) {
                                nd.unset(w);
                                if (w == c)
                                    continue;

                                if (k_ >= 3 && pattern_graphs.at(k_ + 2).adjacent(v, w))
                                    pattern_graphs.at(k_ + 3).add_edge(v, w);
                                else if (k_ >= 2 && pattern_graphs.at(k_ + 1).adjacent(v, w))
                                    pattern_graphs.at(k_ + 2).add_edge(v, w);
                                else if (k_ >= 1)
                                    pattern_graphs.at(k_ + 1).add_edge(v, w);
                            }
                        }
                    }
                }
            }

            for (int g = 1 ; g < max_graphs ; ++g)
                target_graphs.at(g).resize(target_size);

            if (l_ >= 2) {
                for (unsigned v = 0 ; v < target_size ; ++v) {
                    auto nv = target_graphs.at(0).neighbourhood(v);
                    for (int c = nv.first_set_bit() ; c != -1 ; c = nv.first_set_bit()) {
                        nv.unset(c);
                        auto nc = target_graphs.at(0).neighbourhood(c);
                        for (int w = nc.first_set_bit() ; w != -1 && unsigned(w) <= v ; w = nc.first_set_bit()) {
                            nc.unset(w);
                            if (k_ >= 3 && target_graphs.at(2).adjacent(v, w))
                                target_graphs.at(3).add_edge(v, w);
                            else if (k_ >= 2 && target_graphs.at(1).adjacent(v, w))
                                target_graphs.at(2).add_edge(v, w);
                            else if (k_ >= 1)
                                target_graphs.at(1).add_edge(v, w);
                        }
                    }
                }
            }

            if (l_ >= 3) {
                for (unsigned v = 0 ; v < target_size ; ++v) {
                    auto nv = target_graphs.at(0).neighbourhood(v);
                    for (int c = nv.first_set_bit() ; c != -1 ; c = nv.first_set_bit()) {
                        nv.unset(c);
                        auto nc = target_graphs.at(0).neighbourhood(c);
                        for (int d = nc.first_set_bit() ; d != -1 ; d = nc.first_set_bit()) {
                            nc.unset(d);
                            if (unsigned(d) == v)
                                continue;

                            auto nd = target_graphs.at(0).neighbourhood(d);
                            for (int w = nd.first_set_bit() ; w != -1 && unsigned(w) <= v ; w = nd.first_set_bit()) {
                                nd.unset(w);
                                if (w == c)
                                    continue;

                                if (k_ >= 3 && target_graphs.at(k_ + 2).adjacent(v, w))
                                    target_graphs.at(k_ + 3).add_edge(v, w);
                                else if (k_ >= 2 && target_graphs.at(k_ + 1).adjacent(v, w))
                                    target_graphs.at(k_ + 2).add_edge(v, w);
                                else if (k_ >= 1)
                                    target_graphs.at(k_ + 1).add_edge(v, w);
                            }
                        }
                    }
                }
            }
        }

        auto parallel_build_supplemental_graphs(int n_threads) -> void
        {
            auto & max_graphs = static_cast<Parent_ *>(this)->max_graphs;
            auto & pattern_size = static_cast<Parent_ *>(this)->pattern_size;
            auto & target_size = static_cast<Parent_ *>(this)->target_size;
            auto & pattern_graphs = static_cast<Parent_ *>(this)->pattern_graphs;
            auto & target_graphs = static_cast<Parent_ *>(this)->target_graphs;
            auto & tasks = static_cast<Parent_ *>(this)->tasks;

            for (int g = 1 ; g < max_graphs ; ++g)
                pattern_graphs.at(g).resize(pattern_size);

            for (int g = 1 ; g < max_graphs ; ++g)
                target_graphs.at(g).resize(target_size);

            std::atomic<unsigned> pos2p, pos3p, pos2t, pos3t;
            pos2p = 0; pos3p = 0; pos2t = 0; pos3t = 0;

            for (int t = 0 ; t < n_threads ; ++t) {
                tasks.add([&] {
                    if (l_ >= 2) {
                        for (unsigned v ; ((v = pos2p++)) < pattern_size ; ) {
                            auto nv = pattern_graphs.at(0).neighbourhood(v);
                            for (int c = nv.first_set_bit() ; c != -1 ; c = nv.first_set_bit()) {
                                nv.unset(c);
                                auto nc = pattern_graphs.at(0).neighbourhood(c);
                                for (int w = nc.first_set_bit() ; w != -1 && unsigned(w) <= v ; w = nc.first_set_bit()) {
                                    nc.unset(w);
                                    if (k_ >= 3 && pattern_graphs.at(2).adjacent(v, w))
                                        pattern_graphs.at(3).add_edge_atomic(v, w);
                                    else if (k_ >= 2 && pattern_graphs.at(1).adjacent(v, w))
                                        pattern_graphs.at(2).add_edge_atomic(v, w);
                                    else if (k_ >= 1)
                                        pattern_graphs.at(1).add_edge_atomic(v, w);
                                }
                            }
                        }
                    }

                    if (l_ >= 3) {
                        for (unsigned v ; ((v = pos3p++)) < pattern_size ; ) {
                            auto nv = pattern_graphs.at(0).neighbourhood(v);
                            for (int c = nv.first_set_bit() ; c != -1 ; c = nv.first_set_bit()) {
                                nv.unset(c);
                                auto nc = pattern_graphs.at(0).neighbourhood(c);
                                for (int d = nc.first_set_bit() ; d != -1 ; d = nc.first_set_bit()) {
                                    nc.unset(d);
                                    if (unsigned(d) == v)
                                        continue;

                                    auto nd = pattern_graphs.at(0).neighbourhood(d);
                                    for (int w = nd.first_set_bit() ; w != -1 && unsigned(w) <= v ; w = nd.first_set_bit()) {
                                        nd.unset(w);
                                        if (w == c)
                                            continue;

                                        if (k_ >= 3 && pattern_graphs.at(k_ + 2).adjacent(v, w))
                                            pattern_graphs.at(k_ + 3).add_edge_atomic(v, w);
                                        else if (k_ >= 2 && pattern_graphs.at(k_ + 1).adjacent(v, w))
                                            pattern_graphs.at(k_ + 2).add_edge_atomic(v, w);
                                        else if (k_ >= 1)
                                            pattern_graphs.at(k_ + 1).add_edge_atomic(v, w);
                                    }
                                }
                            }
                        }
                    }

                    if (l_ >= 2) {
                        for (unsigned v ; ((v = pos2t++)) < target_size ; ) {
                            auto nv = target_graphs.at(0).neighbourhood(v);
                            for (int c = nv.first_set_bit() ; c != -1 ; c = nv.first_set_bit()) {
                                nv.unset(c);
                                auto nc = target_graphs.at(0).neighbourhood(c);
                                for (int w = nc.first_set_bit() ; w != -1 && unsigned(w) <= v ; w = nc.first_set_bit()) {
                                    nc.unset(w);
                                    if (k_ >= 3 && target_graphs.at(2).adjacent(v, w))
                                        target_graphs.at(3).add_edge_atomic(v, w);
                                    else if (k_ >= 2 && target_graphs.at(1).adjacent(v, w))
                                        target_graphs.at(2).add_edge_atomic(v, w);
                                    else if (k_ >= 1)
                                        target_graphs.at(1).add_edge_atomic(v, w);
                                }
                            }
                        }
                    }

                    if (l_ >= 3) {
                        for (unsigned v ; ((v = pos3t++)) < target_size ; ) {
                            auto nv = target_graphs.at(0).neighbourhood(v);
                            for (int c = nv.first_set_bit() ; c != -1 ; c = nv.first_set_bit()) {
                                nv.unset(c);
                                auto nc = target_graphs.at(0).neighbourhood(c);
                                for (int d = nc.first_set_bit() ; d != -1 ; d = nc.first_set_bit()) {
                                    nc.unset(d);
                                    if (unsigned(d) == v)
                                        continue;

                                    auto nd = target_graphs.at(0).neighbourhood(d);
                                    for (int w = nd.first_set_bit() ; w != -1 && unsigned(w) <= v ; w = nd.first_set_bit()) {
                                        nd.unset(w);
                                        if (w == c)
                                            continue;

                                        if (k_ >= 3 && target_graphs.at(k_ + 2).adjacent(v, w))
                                            target_graphs.at(k_ + 3).add_edge_atomic(v, w);
                                        else if (k_ >= 2 && target_graphs.at(k_ + 1).adjacent(v, w))
                                            target_graphs.at(k_ + 2).add_edge_atomic(v, w);
                                        else if (k_ >= 1)
                                            target_graphs.at(k_ + 1).add_edge_atomic(v, w);
                                    }
                                }
                            }
                        }
                    }
                });
            }

            tasks.complete();
        }
    };
}

#endif
