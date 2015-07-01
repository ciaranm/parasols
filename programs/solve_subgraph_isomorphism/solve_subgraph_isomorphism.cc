/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <graph/graph.hh>
#include <graph/file_formats.hh>
#include <graph/orders.hh>

#include <subgraph_isomorphism/algorithms.hh>

#include <boost/program_options.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace parasols;
namespace po = boost::program_options;

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                  "Display help information")
            ("threads",            po::value<int>(),  "Number of threads to use (where relevant)")
            ("timeout",            po::value<int>(),  "Abort after this many seconds")
            ("format",             po::value<std::string>(), "Specify the format of the input")
            ("verify",                                "Verify that we have found a valid result (for sanity checking changes)")
            ("delete-loops",                          "Discard loops in the input")
            ("induced",                               "Find induced isomorphisms")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("algorithm",    "Specify which algorithm to use")
            ("pattern-file", "Specify the pattern file (DIMACS format, unless --format is specified)")
            ("target-file",  "Specify the target file (DIMACS format, unless --format is specified)")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("algorithm", 1)
            .add("pattern-file", 1)
            .add("target-file", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm pattern target" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("algorithm") || ! options_vars.count("pattern-file") || ! options_vars.count("target-file")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm pattern target" << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an algorithm string name into a runnable function. */
        auto algorithm = subgraph_isomorphism_algorithms.begin(), algorithm_end = subgraph_isomorphism_algorithms.end();
        for ( ; algorithm != algorithm_end ; ++algorithm)
            if (algorithm->first == options_vars["algorithm"].as<std::string>())
                break;

        /* Unknown algorithm? Show a message and exit. */
        if (algorithm == algorithm_end) {
            std::cerr << "Unknown algorithm " << options_vars["algorithm"].as<std::string>() << ", choose from:";
            for (auto a : subgraph_isomorphism_algorithms)
                std::cerr << " " << a.first;
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Figure out what our options should be. */
        SubgraphIsomorphismParams params;

        if (options_vars.count("threads"))
            params.n_threads = options_vars["threads"].as<int>();
        else
            params.n_threads = std::thread::hardware_concurrency();

        if (options_vars.count("delete-loops"))
            params.delete_loops = true;

        params.induced = options_vars.count("induced");

        /* Turn a format name into a runnable function. */
        auto format = graph_file_formats.begin(), format_end = graph_file_formats.end();
        if (options_vars.count("format"))
            for ( ; format != format_end ; ++format)
                if (format->first == options_vars["format"].as<std::string>())
                    break;

        /* Unknown format? Show a message and exit. */
        if (format == format_end) {
            std::cerr << "Unknown format " << options_vars["format"].as<std::string>() << ", choose from:";
            for (auto a : graph_file_formats)
                std::cerr << " " << a.first;
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Read in the graphs */
        auto graphs = std::make_pair(
            std::get<1>(*format)(options_vars["pattern-file"].as<std::string>(), GraphOptions::AllowLoops),
            std::get<1>(*format)(options_vars["target-file"].as<std::string>(), GraphOptions::AllowLoops));

        /* Do the actual run. */
        bool aborted = false;
        auto result = run_this(algorithm->second)(
                graphs,
                params,
                aborted,
                options_vars.count("timeout") ? options_vars["timeout"].as<int>() : 0);

        /* Stop the clock. */
        auto overall_time = duration_cast<milliseconds>(steady_clock::now() - params.start_time);

        /* Display the results. */
        std::cout << std::boolalpha << ! result.isomorphism.empty() << " " << result.nodes;

        if (aborted)
            std::cout << " aborted";
        std::cout << std::endl;

        for (auto v : result.isomorphism)
            std::cout << "(" << graphs.first.vertex_name(v.first) << " -> " << graphs.second.vertex_name(v.second) << ") ";
        std::cout << std::endl;

        std::cout << overall_time.count();
        if (! result.times.empty()) {
            for (auto t : result.times)
                std::cout << " " << t.count();
        }
        std::cout << std::endl;

        if (options_vars.count("verify") && ! result.isomorphism.empty()) {
            for (int i = 0 ; i < graphs.first.size() ; ++i) {
                for (int j = 0 ; j < graphs.first.size() ; ++j) {
                    if (i != j || ! params.delete_loops) {
                        if (graphs.first.adjacent(i, j)) {
                            if (! graphs.second.adjacent(result.isomorphism.find(i)->second, result.isomorphism.find(j)->second)) {
                                std::cerr << "Oops! not an isomorphism" << std::endl;
                                return EXIT_FAILURE;
                            }
                        }
                        else if (params.induced) {
                            if (graphs.second.adjacent(result.isomorphism.find(i)->second, result.isomorphism.find(j)->second)) {
                                std::cerr << "Oops! not an induced isomorphism" << std::endl;
                                return EXIT_FAILURE;
                            }
                        }
                    }
                }
            }
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

