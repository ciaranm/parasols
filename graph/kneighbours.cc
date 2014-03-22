/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/kneighbours.hh>

using namespace parasols;

KNeighbours::KNeighbours(const Graph & graph, const int nk, const std::vector<int> * maybe_restrict) :
    vertices(graph.size())
{
    for (auto & d : vertices) {
        d.firsts.resize(nk + 1);
        for (auto & f : d.firsts)
            f = -1;

        d.distances.resize(graph.size());
    }

    /* build up distance 1 lists */
    for (int i = 0 ; i < graph.size() ; ++i) {
        if (maybe_restrict && ! maybe_restrict->at(i))
            continue;

        int prev = -1;
        for (int j = 0 ; j < graph.size() ; ++j) {
            if (maybe_restrict && ! maybe_restrict->at(j))
                continue;

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
    for (int k = 2 ; k <= nk ; ++k) {
        for (int i = 0 ; i < graph.size() ; ++i) {
            if (maybe_restrict && ! maybe_restrict->at(i))
                continue;

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

