/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_common_subgraph/algorithms.hh>
#include <max_clique/algorithms.hh>

#include <graph/orders.hh>

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

auto algorithms = {
    MaxCliqueAlgorithm{ cco_max_clique<CCOPermutations::None, CCOInference::None> }
};

void table(int size1, int size2, int samples, const MaxCliqueOrderFunction & order_function)
{
    using namespace std::placeholders;

    for (int p1 = 0 ; p1 <= 100 ; p1 += 1) {
        for (int p2 = 0 ; p2 <= 100 ; p2 += 1) {
            std::cout << p1 / 100.0 << " " << p2 / 100.0;

            for (auto & algorithm : algorithms) {
                double size_total = 0.0, nodes_total = 0.0, best_nodes_total = 0.0, first = 0.0, second = 0.0;

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
                }

                std::cout << " " << (size_total / samples) << " " <<
                    (nodes_total / samples) << " " << (best_nodes_total /
                            samples) << " " << first << " " << second;
            }

            std::cout << std::endl;
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
            ("order",      "Specify the initial vertex order")
            ;

        po::positional_options_description positional_options;
        positional_options
            .add("size1",      1)
            .add("size2",      1)
            .add("samples",    1)
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
            std::cout << "Usage: " << argv[0] << " [options] size1 size2 samples order" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No values specified? Show a message and exit. */
        if (! options_vars.count("size1") || ! options_vars.count("size2") || ! options_vars.count("samples")
                || ! options_vars.count("order")) {
            std::cout << "Usage: " << argv[0] << " [options] size1 size2 samples order" << std::endl;
            return EXIT_FAILURE;
        }

        int size1 = options_vars["size1"].as<int>();
        int size2 = options_vars["size2"].as<int>();
        int samples = options_vars["samples"].as<int>();

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

        table(size1, size2, samples, order_function);

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


