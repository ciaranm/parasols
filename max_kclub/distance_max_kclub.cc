/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_kclub/distance_max_kclub.hh>
#include <max_kclub/print_incumbent.hh>
#include <graph/is_club.hh>
#include <algorithm>
#include <limits>

using namespace parasols;

namespace
{
    struct VertexDistance
    {
        int distance;
        int next;
    };

    struct VertexInformation
    {
        std::vector<int> firsts;
        std::vector<VertexDistance> distances;
    };

    struct Distances
    {
        std::vector<VertexInformation> vertices;

        Distances(const Graph & graph, const MaxKClubParams & params) :
            vertices(graph.size())
        {
            for (auto & d : vertices) {
                d.firsts.resize(params.k + 1);
                for (auto & f : d.firsts)
                    f = -1;

                d.distances.resize(graph.size());
            }

            /* build up distance 1 lists */
            for (int i = 0 ; i < graph.size() ; ++i) {
                int prev = -1;
                for (int j = 0 ; j < graph.size() ; ++j) {
                    if (i == j) {
                        vertices[i].firsts[0] = i;
                        vertices[i].distances[j].distance = 0;
                        vertices[i].distances[j].next = -1;
                    }
                    else if (graph.adjacent(i, j)) {
                        if (-1 == vertices[i].firsts[1]) {
                            vertices[i].firsts[1] = j;
                            vertices[i].distances[j].distance = 1;
                            vertices[i].distances[j].next = -1;
                            prev = j;
                        }
                        else {
                            vertices[i].distances[prev].next = j;
                            vertices[i].distances[j].distance = 1;
                            vertices[i].distances[j].next = -1;
                            prev = j;
                        }
                    }
                    else {
                        vertices[i].distances[j].distance = -1;
                        vertices[i].distances[j].next = -1;
                    }
                }
            }

            /* build up distance k lists */
            for (unsigned k = 2 ; k <= params.k ; ++k) {
                for (int i = 0 ; i < graph.size() ; ++i) {
                    int prev = -1;
                    for (int a = vertices[i].firsts[k - 1] ; a != -1 ; a = vertices[i].distances[a].next) {
                        for (int j = vertices[a].firsts[1] ; j != -1 ; j = vertices[a].distances[j].next) {
                            if (vertices[i].distances[j].distance == -1) {
                                vertices[i].distances[j].distance = k;
                                if (-1 == prev)
                                    vertices[i].firsts[k] = j;
                                else
                                    vertices[i].distances[prev].next = j;
                                prev = j;
                            }
                        }
                    }
                }
            }
        }
    };

    auto bound(
            const Graph & graph,
            const Distances & distances,
            std::vector<int> & p) -> int
    {
        std::vector<std::vector<int> > colour_classes((graph.size()));
        int n_colours = 0;

        for (auto & v : p) {
            bool coloured = false;
            for (int c = 0 ; c < graph.size() && ! coloured ; ++c) {
                bool conflicts = false;
                for (auto & w : colour_classes[c]) {
                    if (-1 != distances.vertices[v].distances[w].distance) {
                        conflicts = true;
                        break;
                    }
                }

                if (! conflicts) {
                    colour_classes[c].push_back(v);
                    if (n_colours < c + 1)
                        n_colours = c + 1;
                    coloured = true;
                }
            }

            if (! coloured)
                throw 0;
        }

        return n_colours;
    }

    auto expand(
            const Graph & graph,
            const Distances & distances,
            std::vector<int> & c,                    // current candidate clique
            std::vector<int> & p,                    // potential additions
            std::vector<int> & o,
            std::vector<int> & position,
            MaxKClubResult & result,
            const MaxKClubParams & params) -> void
    {
        ++result.nodes;

        while (! p.empty()) {
            ++position.back();

            if (c.size() + bound(graph, distances, p) <= result.size || result.size >= params.stop_after_finding || params.abort.load())
                return;

            auto v = p.back();
            p.pop_back();

            // consider v in c
            c.push_back(v);

            // club?
            if (c.size() > result.size) {
                if (is_club(graph, params.k, std::set<int>{ c.begin(), c.end() })) {
                    unsigned old_size = result.size;
                    result.size = c.size();
                    result.members.clear();
                    for (auto & i : c)
                        result.members.insert(o[i]);
                    print_incumbent(params, result.size, old_size, true, position);
                }
                else
                    print_incumbent(params, c.size(), result.size, false, position);
            }

            if (! p.empty()) {
                std::vector<int> new_p;
                new_p.reserve(p.size());
                for (auto & w : p) {
                    if (distances.vertices[v].distances[w].distance > 0)
                        new_p.push_back(w);
                }

                if (! new_p.empty()) {
                    position.push_back(0);
                    expand(graph, distances, c, new_p, o, position, result, params);
                    position.pop_back();
                }
            }

            // consider v notin c
            c.pop_back();
        }
    }
}

auto parasols::distance_max_kclub(const Graph & graph, const MaxKClubParams & params) -> MaxKClubResult
{
    MaxKClubResult result;
    result.size = params.initial_bound;

    std::vector<int> c; // current candidate clique
    c.reserve(graph.size());

    std::vector<int> p(graph.size()); // potential additions
    std::iota(p.begin(), p.end(), 0);

    std::vector<int> position;
    position.reserve(graph.size());
    position.push_back(0);

    std::vector<int> o(graph.size()); // vertex ordering

    // populate our order with every vertex initially
    std::iota(o.begin(), o.end(), 0);
    params.order_function(graph, o);

    // re-encode graph as a bit graph
    Graph bit_graph(graph.size(), false);

    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (graph.adjacent(o[i], o[j]))
                bit_graph.add_edge(i, j);

    Distances distances(bit_graph, params);
    expand(bit_graph, distances, c, p, o, position, result, params);

    return result;
}

