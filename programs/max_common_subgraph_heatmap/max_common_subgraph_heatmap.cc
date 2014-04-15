/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_common_subgraph/algorithms.hh>
#include <max_clique/algorithms.hh>

#include <graph/degree_sort.hh>

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
    MaxCliqueAlgorithm{ cco_max_clique<CCOPermutations::None, CCOInference::None> },
    MaxCliqueAlgorithm{ cco_max_clique<CCOPermutations::None, CCOInference::LazyGlobalDomination> }
};

void table(int size1, int size2, int samples)
{
    using namespace std::placeholders;

    for (int p1 = 0 ; p1 <= 100 ; p1 += 1) {
        for (int p2 = 0 ; p2 <= 100 ; p2 += 1) {
            std::cout << p1 / 100.0 << " " << p2 / 100.0;

            for (auto & algorithm : algorithms) {
                double size_total = 0.0, nodes_total = 0.0;

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
                    params.order_function = std::bind(degree_sort, _1, _2, false);
                    std::atomic<bool> abort;
                    abort.store(false);
                    params.abort = &abort;
                    params.start_time = steady_clock::now();

                    auto result = clique_max_common_subgraph(std::make_pair(graph1, graph2), params);
                    size_total += result.size;
                    nodes_total += result.nodes;
                }

                std::cout << " " << (size_total / samples) << " " << (nodes_total / samples);
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
            ;

        po::positional_options_description positional_options;
        positional_options
            .add("size1",      1)
            .add("size2",      1)
            .add("samples",    1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] size1 size2 samples" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No values specified? Show a message and exit. */
        if (! options_vars.count("size1") || ! options_vars.count("size2") || ! options_vars.count("samples")) {
            std::cout << "Usage: " << argv[0] << " [options] size1 size2 samples" << std::endl;
            return EXIT_FAILURE;
        }

        int size1 = options_vars["size1"].as<int>();
        int size2 = options_vars["size2"].as<int>();
        int samples = options_vars["samples"].as<int>();

        table(size1, size2, samples);

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


