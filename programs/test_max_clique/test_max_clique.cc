/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/max_clique_params.hh>
#include <max_clique/max_clique_result.hh>
#include <max_clique/algorithms.hh>

#include <graph/degree_sort.hh>

#include <boost/program_options.hpp>

#include <iostream>
#include <iomanip>
#include <exception>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <thread>

using namespace parasols;
namespace po = boost::program_options;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

std::mt19937 rnd;

bool compare(int size, int samples,
        const std::function<MaxCliqueResult (const Graph &, const MaxCliqueParams &)> & algorithm1,
        const std::function<MaxCliqueResult (const Graph &, const MaxCliqueParams &)> & algorithm2)
{
    using namespace std::placeholders;

    bool ok = true;

    for (int p = 1 ; p < 100 ; ++p) {
        std::cerr << p << " ";

        for (int n = 0 ; n < samples ; ++n) {
            Graph graph(size, false);

            std::uniform_real_distribution<double> dist(0.0, 1.0);
            for (int e = 0 ; e < size ; ++e)
                for (int f = e + 1 ; f < size ; ++f)
                    if (dist(rnd) <= (double(p) / 100.0))
                        graph.add_edge(e, f);

            MaxCliqueParams params1;
            params1.order_function = std::bind(degree_sort, _1, _2, false);
            params1.original_graph = &graph;
            params1.abort.store(false);
            params1.start_time = steady_clock::now();
            params1.n_threads = std::thread::hardware_concurrency();
            MaxCliqueResult result1 = algorithm1(graph, params1);

            MaxCliqueParams params2;
            params2.order_function = std::bind(degree_sort, _1, _2, false);
            params2.original_graph = &graph;
            params2.abort.store(false);
            params2.start_time = steady_clock::now();
            params2.n_threads = std::thread::hardware_concurrency();
            MaxCliqueResult result2 = algorithm2(graph, params2);

            if (result1.size != result2.size) {
                std::cerr << "Error! Got " << result1.size << " and " << result2.size << " for "
                    << p << " " << n << std::endl;
                ok = false;
            }
        }
    }

    std::cerr << std::endl;
    return ok;
}

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                  "Display help information")
            ;

        po::options_description all_options{ "All options" };
        all_options.add(display_options);

        all_options.add_options()
            ("algorithm1",  po::value<std::string>(),  "Algorithm 1")
            ("algorithm2",  po::value<std::string>(),  "Algorithm 2")
            ("size",        po::value<int>(),          "Size of graph")
            ("samples",     po::value<int>(),          "Sample size")
            ;

        po::positional_options_description positional_options;
        positional_options
            .add("algorithm1",   1)
            .add("algorithm2",   1)
            .add("size",         1)
            .add("samples",      1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm1 algorithm2 size samples" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No values specified? Show a message and exit. */
        if (! options_vars.count("algorithm1") || ! options_vars.count("algorithm2")
                || ! options_vars.count("size") || ! options_vars.count("samples")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm1 algorithm2 size samples" << std::endl;
            return EXIT_FAILURE;
        }

        int size = options_vars["size"].as<int>();
        int samples = options_vars["samples"].as<int>();

        /* Turn an algorithm string name into a runnable function. */
        auto algorithm1 = max_clique_algorithms.begin(), algorithm1_end = max_clique_algorithms.end();
        for ( ; algorithm1 != algorithm1_end ; ++algorithm1)
            if (std::get<0>(*algorithm1) == options_vars["algorithm1"].as<std::string>())
                break;

        if (algorithm1 == algorithm1_end) {
            std::cerr << "Unknown algorithm " << options_vars["algorithm1"].as<std::string>() << ", choose from:";
            for (auto a : max_clique_algorithms)
                std::cerr << " " << std::get<0>(a);
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        auto algorithm2 = max_clique_algorithms.begin(), algorithm2_end = max_clique_algorithms.end();
        for ( ; algorithm2 != algorithm2_end ; ++algorithm2)
            if (std::get<0>(*algorithm2) == options_vars["algorithm2"].as<std::string>())
                break;

        if (algorithm2 == algorithm2_end) {
            std::cerr << "Unknown algorithm " << options_vars["algorithm2"].as<std::string>() << ", choose from:";
            for (auto a : max_clique_algorithms)
                std::cerr << " " << std::get<0>(a);
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        if (! compare(size, samples, std::get<1>(*algorithm1), std::get<1>(*algorithm2))) {
            std::cerr << "Uh oh. Comparison failed." << std::endl;
            return EXIT_FAILURE;
        }

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

