/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <boost/program_options.hpp>

#include <graph/graph.hh>
#include <graph/product.hh>
#include <graph/file_formats.hh>

#include <subgraph_isomorphism/algorithms.hh>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <set>
#include <random>

using namespace parasols;
namespace po = boost::program_options;

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                         "Display help information")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("pattern-size", po::value<int>(), "Number of vertices in the pattern graph")
            ("target-size",  po::value<int>(), "Number of vertices in the target graph")
            ("induced",                        "Create an induced association")
            ("solve",                          "Solve, rather than displaying the density")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("pattern-size", 1)
            .add("target-size", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] pattern-size target-size" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No n specified? Show a message and exit. */
        if (! options_vars.count("pattern-size") || ! options_vars.count("target-size")) {
            std::cout << "Usage: " << argv[0] << " [options] pattern-size target-size" << std::endl;
            return EXIT_FAILURE;
        }

        int pattern_size = options_vars["pattern-size"].as<int>();
        int target_size = options_vars["target-size"].as<int>();

        std::mt19937 rand;
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        for (int td = 0 ; td <= 100 ; ++td) {
            double target_density = td / 100.0;

            Graph target_graph(target_size, false);
            for (int e = 0 ; e < target_size ; ++e)
                for (int f = e + 1 ; f < target_size ; ++f)
                    if (dist(rand) <= target_density)
                        target_graph.add_edge(e, f);

            for (int pd = 0 ; pd <= 100 ; ++pd) {
                double pattern_density = pd / 100.0;

                Graph pattern_graph(pattern_size, false);
                for (int e = 0 ; e < pattern_size ; ++e)
                    for (int f = e + 1 ; f < pattern_size ; ++f)
                        if (dist(rand) <= pattern_density)
                            pattern_graph.add_edge(e, f);

                if (options_vars.count("solve")) {
                    SubgraphIsomorphismParams params;
                    std::atomic<bool> abort{ false };
                    params.abort = &abort;
                    params.induced = options_vars.count("induced");

                    auto algorithm = std::find_if(
                            subgraph_isomorphism_algorithms.begin(),
                            subgraph_isomorphism_algorithms.end(),
                            [] (const auto & a) { return a.first == "gbbj"; })->second;

                    auto result = algorithm(std::make_pair(pattern_graph, target_graph), params);

                    std::cout << (result.isomorphism.empty() ? "-" : "") << result.nodes << " " << std::flush;
                }
                else {
                    auto product = options_vars.count("induced") ?
                        modular_product(pattern_graph, target_graph) :
                        noninduced_modular_product(pattern_graph, target_graph);

                    unsigned long long edges_seen = 0, total_edges = 0;

                    for (int e = 0 ; e < product.size() ; ++e)
                        for (int f = 0 ; f < product.size() ; ++f) {
                            ++total_edges;
                            if (product.adjacent(e, f))
                                ++edges_seen;
                        }

                    std::cout << (edges_seen + 0.0) / (total_edges + 0.0) << " " << std::flush;
                }
            }
            std::cout << std::endl;
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



