/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_common_subgraph/algorithms.hh>
#include <max_clique/algorithms.hh>

#include <graph/orders.hh>
#include <graph/product.hh>

#include <boost/program_options.hpp>

#include <iostream>
#include <iomanip>
#include <exception>
#include <algorithm>
#include <random>
#include <thread>
#include <cstdlib>

using namespace parasols;
namespace po = boost::program_options;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

void table(int size1, int size2, int samples,
        const MaxCliqueAlgorithm & algorithm,
        const MaxCliqueOrderFunction & order_function)
{
    using namespace std::placeholders;

    for (int p1 = 0 ; p1 <= 100 ; p1 += 1) {
        for (int p2 = 0 ; p2 <= 100 ; p2 += 1) {
            std::cout << p1 / 100.0 << " " << p2 / 100.0;

            double size_total = 0.0, nodes_total = 0.0, best_nodes_total = 0.0, first = 0.0, second = 0.0,
                   degree_spread = 0.0, find_nodes_total = 0.0, prove_nodes_total = 0.0, degree_sum_total = 0.0;

            for (int sample = 0 ; sample < samples ; ++sample) {
                std::mt19937 rnd1, rnd2;
                rnd1.seed(1234 + sample + (p1 * 2 * samples));
                rnd2.seed(24681012 + sample + samples + (p2 * 2 * samples));
                Graph graph1(size1, false), graph2(size2, false);

                std::uniform_real_distribution<double> dist(0.0, 1.0);
                for (int e = 0 ; e < size1 ; ++e)
                    for (int f = e + 1 ; f < size1 ; ++f)
                        if (dist(rnd1) <= (double(p1) / 100.0))
                            graph1.add_edge(e, f);

                for (int e = 0 ; e < size2 ; ++e)
                    for (int f = e + 1 ; f < size2 ; ++f)
                        if (dist(rnd2) <= (double(p2) / 100.0))
                            graph2.add_edge(e, f);

                MaxCommonSubgraphParams params;
                params.max_clique_algorithm = algorithm;
                params.order_function = order_function;
                std::atomic<bool> abort;
                abort.store(false);
                params.abort = &abort;
                params.start_time = steady_clock::now();

                auto result1 = clique_max_common_subgraph(std::make_pair(graph1, graph2), params);
                size_total += result1.size;
                nodes_total += result1.nodes;

                params.start_time = steady_clock::now();
                auto result2 = clique_max_common_subgraph(std::make_pair(graph2, graph1), params);

                if (result1.size != result2.size)
                    throw 0; /* oops... */

                if (result1.nodes < result2.nodes) {
                    best_nodes_total += result1.nodes;
                    first += 1.0;
                }
                else if (result1.nodes == result2.nodes) {
                    best_nodes_total += result1.nodes;
                    first += 0.5;
                    second += 0.5;
                }
                else {
                    best_nodes_total += result2.nodes;
                    second += 1.0;
                }

                params.start_time = steady_clock::now();
                params.stop_after_finding = result1.size;
                auto result3 = clique_max_common_subgraph(std::make_pair(graph1, graph2), params);

                find_nodes_total += result3.nodes;

                params.start_time = steady_clock::now();
                params.stop_after_finding = std::numeric_limits<unsigned>::max();
                params.initial_bound = result1.size;
                auto result4 = clique_max_common_subgraph(std::make_pair(graph1, graph2), params);

                prove_nodes_total += result4.nodes;

                auto product = modular_product(graph1, graph2);
                std::vector<unsigned> degrees(product.size(), 0);
                for (int v = 0 ; v < product.size() ; ++v) {
                    degree_sum_total += product.degree(v);
                    degrees[product.degree(v)]++;
                }

                for (auto & d : degrees)
                    if (0 != d)
                        degree_spread += 1.0;
            }

            std::cout << " " << (size_total / samples) << " "
                << (nodes_total / samples) << " " << (best_nodes_total / samples)
                << " " << first << " " << second << " " << (degree_spread / samples)
                << " " << (find_nodes_total / samples) << " " << (prove_nodes_total / samples)
                << " " << (degree_sum_total / samples)
                << std::endl;
        }

        std::cout << std::endl;
    }
}

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                 "Display help information")
            ;

        po::options_description all_options{ "All options" };
        all_options.add(display_options);

        all_options.add_options()
            ("size1",       po::value<int>(),                       "Size of first graph")
            ("size2",       po::value<int>(),                       "Size of second graph")
            ("samples",     po::value<int>(),                       "Sample size")
            ("algorithm",   "Specify the clique algorithm")
            ("order",       "Specify the initial vertex order")
            ;

        po::positional_options_description positional_options;
        positional_options
            .add("size1",      1)
            .add("size2",      1)
            .add("samples",    1)
            .add("algorithm",  1)
            .add("order",      1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] size1 size2 samples algorithm order" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No values specified? Show a message and exit. */
        if (! options_vars.count("size1") || ! options_vars.count("size2") || ! options_vars.count("samples")
                || ! options_vars.count("algorithm") || ! options_vars.count("order")) {
            std::cout << "Usage: " << argv[0] << " [options] size1 size2 samples algorithm order" << std::endl;
            return EXIT_FAILURE;
        }

        int size1 = options_vars["size1"].as<int>();
        int size2 = options_vars["size2"].as<int>();
        int samples = options_vars["samples"].as<int>();

        /* Turn an algorithm string name into a runnable function. */
        auto algorithm = max_clique_algorithms.begin(), algorithm_end = max_clique_algorithms.end();
        for ( ; algorithm != algorithm_end ; ++algorithm)
            if (std::get<0>(*algorithm) == options_vars["algorithm"].as<std::string>())
                break;

        /* Unknown algorithm? Show a message and exit. */
        if (algorithm == algorithm_end) {
            std::cerr << "Unknown algorithm " << options_vars["algorithm"].as<std::string>() << ", choose from:";
            for (auto a : max_clique_algorithms)
                std::cerr << " " << std::get<0>(a);
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an order string name into a runnable function. */
        MaxCliqueOrderFunction order_function;
        for (auto order = orders.begin() ; order != orders.end() ; ++order)
            if (std::get<0>(*order) == options_vars["order"].as<std::string>()) {
                order_function = std::get<1>(*order);
                break;
            }

        /* Unknown algorithm? Show a message and exit. */
        if (! order_function) {
            std::cerr << "Unknown order " << options_vars["order"].as<std::string>() << ", choose from:";
            for (auto a : orders)
                std::cerr << " " << std::get<0>(a);
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        table(size1, size2, samples, std::get<1>(*algorithm), order_function);

        return EXIT_SUCCESS;
    }
    catch (const po::error & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Try " << argv[0] << " --help" << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}


