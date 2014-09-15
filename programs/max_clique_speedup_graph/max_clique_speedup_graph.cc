/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_clique/max_clique_params.hh>
#include <max_clique/max_clique_result.hh>
#include <max_clique/algorithms.hh>

#include <graph/degree_sort.hh>
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

std::mt19937 rnd;

void table(
        int size,
        int samples,
        const MaxCliqueOrderFunction & order_function,
        const std::vector<std::function<MaxCliqueResult (const Graph &, const MaxCliqueParams &)> > & algorithms,
        bool find_prove)
{
    using namespace std::placeholders;

    for (int p = 1 ; p < 100 ; ++p) {
        std::vector<double> omega_average((algorithms.size()));
        std::vector<double> time_average((algorithms.size()));
        std::vector<double> find_time_average((algorithms.size()));
        std::vector<double> prove_time_average((algorithms.size()));
        std::vector<double> nodes_average((algorithms.size()));
        std::vector<double> find_nodes_average((algorithms.size()));
        std::vector<double> prove_nodes_average((algorithms.size()));

        for (int n = 0 ; n < samples ; ++n) {
            Graph graph(size, false);

            std::uniform_real_distribution<double> dist(0.0, 1.0);
            for (int e = 0 ; e < size ; ++e)
                for (int f = e + 1 ; f < size ; ++f)
                    if (dist(rnd) <= (double(p) / 100.0))
                        graph.add_edge(e, f);

            for (unsigned a = 0 ; a < algorithms.size() ; ++a) {
                unsigned omega;
                {
                    MaxCliqueParams params;
                    params.order_function = order_function;
                    params.n_threads = std::thread::hardware_concurrency();
                    params.original_graph = &graph;
                    std::atomic<bool> abort;
                    abort.store(false);
                    params.abort = &abort;

                    params.start_time = steady_clock::now();

                    MaxCliqueResult result = algorithms.at(a)(graph, params);

                    auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

                    omega_average.at(a) += double(result.size) / double(samples);
                    time_average.at(a) += double(overall_time.count()) / double(samples);
                    nodes_average.at(a) += double(result.nodes) / double(samples);
                    omega = result.size;
                }

                if (find_prove) {
                    MaxCliqueParams params;
                    params.order_function = order_function;
                    params.n_threads = std::thread::hardware_concurrency();
                    params.original_graph = &graph;
                    std::atomic<bool> abort;
                    abort.store(false);
                    params.abort = &abort;
                    params.stop_after_finding = omega;

                    params.start_time = steady_clock::now();

                    MaxCliqueResult result = algorithms.at(a)(graph, params);

                    auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

                    find_time_average.at(a) += double(overall_time.count()) / double(samples);
                    find_nodes_average.at(a) += double(result.nodes) / double(samples);
                }

                if (find_prove) {
                    MaxCliqueParams params;
                    params.order_function = order_function;
                    params.n_threads = std::thread::hardware_concurrency();
                    params.original_graph = &graph;
                    std::atomic<bool> abort;
                    abort.store(false);
                    params.abort = &abort;
                    params.initial_bound = omega;

                    params.start_time = steady_clock::now();

                    MaxCliqueResult result = algorithms.at(a)(graph, params);

                    auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

                    prove_time_average.at(a) += double(overall_time.count()) / double(samples);
                    prove_nodes_average.at(a) += double(result.nodes) / double(samples);
                }
            }
        }

        std::cout << (double(p) / 100.0);
        for (unsigned a = 0 ; a < algorithms.size() ; ++a) {
            if (0 == a)
                std::cout << " " << omega_average.at(a);

            if (find_prove)
                std::cout
                    << " " << time_average.at(a)
                    << " " << find_time_average.at(a)
                    << " " << prove_time_average.at(a)
                    << " " << nodes_average.at(a)
                    << " " << find_nodes_average.at(a)
                    << " " << prove_nodes_average.at(a);
            else
                std::cout
                    << " " << time_average.at(a)
                    << " " << nodes_average.at(a);
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
            ("find-prove",           "Include find / prove measurements")
            ;

        po::options_description all_options{ "All options" };
        all_options.add(display_options);

        all_options.add_options()
            ("size",        po::value<int>(),                       "Size of graph")
            ("samples",     po::value<int>(),                       "Sample size")
            ("order",       po::value<std::string>(),               "Vertex ordering")
            ("algorithm",   po::value<std::vector<std::string> >(), "Algorithms")
            ;

        po::positional_options_description positional_options;
        positional_options
            .add("size",         1)
            .add("samples",      1)
            .add("order",        1)
            .add("algorithm",    -1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] size samples order algorithms..." << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No values specified? Show a message and exit. */
        if (! options_vars.count("size") || ! options_vars.count("samples") || ! options_vars.count("order")) {
            std::cout << "Usage: " << argv[0] << " [options] size samples order algorithms..." << std::endl;
            return EXIT_FAILURE;
        }

        int size = options_vars["size"].as<int>();
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

        std::vector<std::function<MaxCliqueResult (const Graph &, const MaxCliqueParams &)> > selected_algorithms;
        for (auto & s : options_vars["algorithm"].as<std::vector<std::string> >()) {
            /* Turn an algorithm string name into a runnable function. */
            auto algorithm = max_clique_algorithms.begin(), algorithm_end = max_clique_algorithms.end();
            for ( ; algorithm != algorithm_end ; ++algorithm)
                if (std::get<0>(*algorithm) == s)
                    break;

            if (algorithm == algorithm_end) {
                std::cerr << "Unknown algorithm " << s << ", choose from:";
                for (auto a : max_clique_algorithms)
                    std::cerr << " " << std::get<0>(a);
                std::cerr << std::endl;
                return EXIT_FAILURE;
            }

            selected_algorithms.push_back(std::get<1>(*algorithm));
        }

        table(size, samples, order_function, selected_algorithms, options_vars.count("find-prove"));

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

