/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <max_biclique/max_biclique_params.hh>
#include <max_biclique/max_biclique_result.hh>

#include <max_biclique/naive_max_biclique.hh>
#include <max_biclique/ccd_max_biclique.hh>
#include <max_biclique/dccd_max_biclique.hh>

#include <boost/program_options.hpp>

#include <iostream>
#include <iomanip>
#include <exception>
#include <algorithm>
#include <random>
#include <thread>
#include <cstdlib>
#include <cmath>

using namespace parasols;
namespace po = boost::program_options;

std::mt19937 rnd;

namespace
{
    auto algorithms = {
        std::make_tuple( std::string{ "naive" },      naive_max_biclique ),
        std::make_tuple( std::string{ "ccd" },        ccd_max_biclique ),
        std::make_tuple( std::string{ "dccd" },       dccd_max_biclique )
    };
}

void table(
        int size,
        int samples,
        const std::vector<std::function<MaxBicliqueResult (const Graph &, const MaxBicliqueParams &)> > & algorithms)
{
    for (int p = 1 ; p < 100 ; ++p) {
        for (int symmetry = 0 ; symmetry <= 1 ; ++symmetry) {
            std::vector<double> omega_average((algorithms.size()));
            std::vector<double> time_average((algorithms.size()));
            std::vector<double> find_time_average((algorithms.size()));
            std::vector<double> prove_time_average((algorithms.size()));
            std::vector<double> nodes_average((algorithms.size()));
            std::vector<double> find_nodes_average((algorithms.size()));
            std::vector<double> prove_nodes_average((algorithms.size()));
            std::vector<double> speedup_average((algorithms.size()));
            std::vector<std::vector<double> > times((samples));
            std::vector<std::vector<double> > speedups((samples));

            for (int n = 0 ; n < samples ; ++n) {
                Graph graph(size, false);

                std::uniform_real_distribution<double> dist(0.0, 1.0);
                for (int e = 0 ; e < size ; ++e)
                    for (int f = e + 1 ; f < size ; ++f)
                        if (dist(rnd) <= (double(p) / 100.0))
                            graph.add_edge(e, f);

                double baseline = 0.0;
                for (unsigned a = 0 ; a < algorithms.size() ; ++a) {
                    unsigned omega;

                    {
                        MaxBicliqueParams params;
                        params.n_threads = 2;
                        params.break_ab_symmetry = symmetry;

                        params.start_time = std::chrono::steady_clock::now();

                        auto result = algorithms.at(a)(graph, params);

                        auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time);

                        omega_average.at(a) += double(result.size) / double(samples);
                        time_average.at(a) += double(overall_time.count()) / double(samples);
                        nodes_average.at(a) += double(result.nodes) / double(samples);
                        times.at(a).push_back(double(overall_time.count()));
                        omega = result.size;

                        if (0 == a)
                            baseline = double(overall_time.count());
                        speedups.at(a).push_back(baseline / double(overall_time.count()));
                        speedup_average.at(a) += (baseline / double(overall_time.count())) / samples;
                    }

                    {
                        MaxBicliqueParams params;
                        params.n_threads = 2;
                        params.abort.store(false);
                        params.stop_after_finding = omega;
                        params.break_ab_symmetry = symmetry;

                        params.start_time = std::chrono::steady_clock::now();

                        auto result = algorithms.at(a)(graph, params);

                        auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time);

                        find_time_average.at(a) += double(overall_time.count()) / double(samples);
                        find_nodes_average.at(a) += double(result.nodes) / double(samples);
                    }

                    {
                        MaxBicliqueParams params;
                        params.n_threads = 2;
                        params.abort.store(false);
                        params.initial_bound = omega;
                        params.break_ab_symmetry = symmetry;

                        params.start_time = std::chrono::steady_clock::now();

                        auto result = algorithms.at(a)(graph, params);

                        auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time);

                        prove_time_average.at(a) += double(overall_time.count()) / double(samples);
                        prove_nodes_average.at(a) += double(result.nodes) / double(samples);
                    }
                }
            }

            if (0 == symmetry)
                std::cout << (double(p) / 100.0);

            for (unsigned a = 0 ; a < algorithms.size() ; ++a) {
                if (0 == a && 0 == symmetry)
                    std::cout << " " << omega_average.at(a);

                double ts = 0.0;
                for (int n = 0 ; n < samples ; ++n)
                    ts += (times.at(a).at(n) - time_average.at(a)) * (times.at(a).at(n) - time_average.at(a));
                ts = std::sqrt(ts / samples);

                double ss = 0.0;
                for (int n = 0 ; n < samples ; ++n)
                    ss += (speedups.at(a).at(n) - speedup_average.at(a)) * (speedups.at(a).at(n) - speedup_average.at(a));
                ss = std::sqrt(ss / samples);

                std::cout
                    << " " << time_average.at(a)
                    << " " << ts
                    << " " << speedup_average.at(a)
                    << " " << ss
                    << " " << find_time_average.at(a)
                    << " " << prove_time_average.at(a)
                    << " " << nodes_average.at(a)
                    << " " << find_nodes_average.at(a)
                    << " " << prove_nodes_average.at(a);
            }
        }

        std::cout << std::endl;
    }
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
            ("size",        po::value<int>(),                       "Size of graph")
            ("samples",     po::value<int>(),                       "Sample size")
            ("algorithm",   po::value<std::vector<std::string> >(), "Algorithms")
            ;

        po::positional_options_description positional_options;
        positional_options
            .add("size",         1)
            .add("samples",      1)
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
            std::cout << "Usage: " << argv[0] << " [options] size samples algorithms..." << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No values specified? Show a message and exit. */
        if (! options_vars.count("size") || ! options_vars.count("samples")) {
            std::cout << "Usage: " << argv[0] << " [options] size samples algorithms..." << std::endl;
            return EXIT_FAILURE;
        }

        int size = options_vars["size"].as<int>();
        int samples = options_vars["samples"].as<int>();

        std::vector<std::function<MaxBicliqueResult (const Graph &, const MaxBicliqueParams &)> > selected_algorithms;
        for (auto & s : options_vars["algorithm"].as<std::vector<std::string> >()) {
            /* Turn an algorithm string name into a runnable function. */
            auto algorithm = algorithms.begin(), algorithm_end = algorithms.end();
            for ( ; algorithm != algorithm_end ; ++algorithm)
                if (std::get<0>(*algorithm) == s)
                    break;

            if (algorithm == algorithm_end) {
                std::cerr << "Unknown algorithm " << s << ", choose from:";
                for (auto a : algorithms)
                    std::cerr << " " << std::get<0>(a);
                std::cerr << std::endl;
                return EXIT_FAILURE;
            }

            selected_algorithms.push_back(std::get<1>(*algorithm));
        }

        table(size, samples, selected_algorithms);

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

