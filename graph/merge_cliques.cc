/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/merge_cliques.hh>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/max_cardinality_matching.hpp>
#include <boost/graph/graph_utility.hpp>

auto parasols::merge_cliques(
        const std::function<bool (int, int)> & adjacent, const std::set<int> & clique1, const std::set<int> & clique2) -> std::set<int>
{
    /* health warning: this code is awful, and was just written to see if the
     * idea is worth doing properly. */

    std::set<int> common;
    std::vector<int> a, b;
    std::set_intersection(clique1.begin(), clique1.end(), clique2.begin(), clique2.end(),
            std::inserter(common, common.begin()));
    std::set_difference(clique1.begin(), clique1.end(), clique2.begin(), clique2.end(),
            std::back_inserter(a));
    std::set_difference(clique2.begin(), clique2.end(), clique1.begin(), clique1.end(),
            std::back_inserter(b));

    std::set<int> merged_clique = common;

    boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> merge_graph(a.size() + b.size());

    for (unsigned i = 0 ; i < a.size() ; ++i)
        for (unsigned j = 0 ; j < b.size() ; ++j)
            if (! adjacent(a[i], b[j])) {
                boost::add_edge(i, a.size() + j, merge_graph);
            }

    std::vector<boost::graph_traits<decltype(merge_graph)>::vertex_descriptor> mate(a.size() + b.size());
    boost::edmonds_maximum_cardinality_matching(merge_graph, &mate[0]);

    std::set<int> matched_vertices;
    std::set<std::pair<int, int> > unmatched_edges;

    for (unsigned i = 0 ; i < a.size() ; ++i)
        for (unsigned j = 0 ; j < b.size() ; ++j)
            if (boost::is_adjacent(merge_graph, i, unsigned(a.size() + j))) {
                unmatched_edges.insert(std::make_pair(i, a.size() + j));
                unmatched_edges.insert(std::make_pair(a.size() + j, i));
            }

    boost::graph_traits<decltype(merge_graph)>::vertex_iterator vi, vi_end;
    for(boost::tie(vi, vi_end) = boost::vertices(merge_graph) ; vi != vi_end ; ++vi)
        if (mate[*vi] != boost::graph_traits<decltype(merge_graph)>::null_vertex()) {
            matched_vertices.insert(*vi);
            matched_vertices.insert(mate[*vi]);
            unmatched_edges.erase(std::make_pair(*vi, mate[*vi]));
        }

    std::set<int> s0, remaining;
    for (unsigned i = 0 ; i < a.size() + b.size() ; ++i)
        if (! matched_vertices.count(i))
            s0.insert(i);
        else
            remaining.insert(i);

    std::set<int> odd, even = s0, all_odd;
    while (! remaining.empty()) {
        /* odd set */
        odd.clear();

        for (auto r = remaining.begin() ; r != remaining.end() ; ) {
            bool add = false;
            for (auto & v : even)
                if (unmatched_edges.count(std::make_pair(*r, v))) {
                    add = true;
                    break;
                }

            if (add) {
                odd.insert(*r);
                remaining.erase(r++);
            }
            else
                ++r;
        }

        if (odd.empty()) {
            odd.insert(*remaining.begin());
            remaining.erase(remaining.begin());
        }

        all_odd.insert(odd.begin(), odd.end());

        /* even set */
        even.clear();

        for (auto & o : odd) {
            even.insert(mate[o]);
            remaining.erase(mate[o]);
        }
    }

    for (unsigned i = 0 ; i < a.size() + b.size() ; ++i)
        if (! all_odd.count(i)) {
            if (i < a.size())
                merged_clique.insert(a[i]);
            else
                merged_clique.insert(b[i - a.size()]);
        }

    return merged_clique;
}

