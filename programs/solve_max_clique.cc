/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <graph/graph.hh>
#include <graph/dimacs.hh>
#include <graph/power.hh>

#include <max_clique/naive_max_clique.hh>
#include <max_clique/mcsa1_max_clique.hh>
#include <max_clique/tmcsa1_max_clique.hh>
#include <max_clique/bmcsa_max_clique.hh>
#include <max_clique/tbmcsa_max_clique.hh>
#include <max_clique/bmcsabin_max_clique.hh>
#include <max_clique/tbmcsabin_max_clique.hh>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <chrono>
#include <thread>

using namespace parasols;
namespace po = boost::program_options;

namespace
{
    auto run_with_power(MaxCliqueResult func(const Graph &, const MaxCliqueParams &)) ->
        std::function<MaxCliqueResult (const Graph &, MaxCliqueParams &, bool &, int)>
    {
        return run_this_wrapped<MaxCliqueResult, MaxCliqueParams, Graph>(
                [func] (const Graph & graph, const MaxCliqueParams & params) -> MaxCliqueResult {
                    if (params.power > 0)
                        return func(power(graph, params.power), params);
                    else
                        return func(graph, params);
                });
    }
}

auto main(int argc, char * argv[]) -> int
{
    auto algorithms = {
        std::make_tuple( std::string{ "naive" },      run_with_power(naive_max_clique), false ),
        std::make_tuple( std::string{ "mcsa1" },      run_with_power(mcsa1_max_clique), false ),
        std::make_tuple( std::string{ "tmcsa1" },     run_with_power(tmcsa1_max_clique), false ),
        std::make_tuple( std::string{ "bmcsa1" },     run_with_power(bmcsa_max_clique<MaxCliqueOrder::Degree>), false ),
        std::make_tuple( std::string{ "bmcsam" },     run_with_power(bmcsa_max_clique<MaxCliqueOrder::Manual>), true ),
        std::make_tuple( std::string{ "bmcsa1bin" },  run_with_power(bmcsabin_max_clique), false ),
        std::make_tuple( std::string{ "tbmcsa1" },    run_with_power(tbmcsa_max_clique<MaxCliqueOrder::Degree>), false ),
        std::make_tuple( std::string{ "tbmcsam" },    run_with_power(tbmcsa_max_clique<MaxCliqueOrder::Manual>), true ),
        std::make_tuple( std::string{ "tbmcsa1bin" }, run_with_power(tbmcsabin_max_clique), false )
    };

    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                 "Display help information")
            ("threads",            po::value<int>(), "Number of threads to use (where relevant)")
            ("stop-after-finding", po::value<int>(), "Stop after finding a clique of this size")
            ("initial-bound",      po::value<int>(), "Specify an initial bound")
            ("manual-order",       po::value<std::string>(),
                "Specify a manual vertex ordering, for bmcsam etc. Must be a list of order:bound pairs "
                "separated by commas.")
            ("print-incumbents",                     "Print new incumbents as they are found")
            ("split-depth",        po::value<int>(), "Specify the depth at which to perform splitting (where relevant)")
            ("work-donation",                        "Enable work donation (where relevant)")
            ("donate-when-idle",                     "Donate work only when idle (where relevant)")
            ("min-donation-size",  po::value<int>(), "Do not donate below this size (where relevant)")
            ("timeout",            po::value<int>(), "Abort after this many seconds")
            ("power",              po::value<int>(), "Raise the graph to this power (to solve s-clique)")
            ("verify",                               "Verify that we have found a valid result (for sanity checking changes)")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("algorithm", "Specify which algorithm to use")
            ("input-file", "Specify the input file (DIMACS format)")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("algorithm", 1)
            .add("input-file", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm file" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("algorithm") || ! options_vars.count("input-file")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm file" << std::endl;
            return EXIT_FAILURE;
        }

        /* Turn an algorithm string name into a runnable function. */
        auto algorithm = algorithms.begin(), algorithm_end = algorithms.end();
        for ( ; algorithm != algorithm_end ; ++algorithm)
            if (std::get<0>(*algorithm) == options_vars["algorithm"].as<std::string>())
                break;

        /* Unknown algorithm? Show a message and exit. */
        if (algorithm == algorithm_end) {
            std::cerr << "Unknown algorithm " << options_vars["algorithm"].as<std::string>() << ", choose from:";
            for (auto a : algorithms)
                std::cerr << " " << std::get<0>(a);
            std::cerr << std::endl;
            return EXIT_FAILURE;
        }

        /* Figure out what our options should be. */
        MaxCliqueParams params;

        if (options_vars.count("threads"))
            params.n_threads = options_vars["threads"].as<int>();
        else
            params.n_threads = std::thread::hardware_concurrency();

        if (options_vars.count("stop-after-finding"))
            params.stop_after_finding = options_vars["stop-after-finding"].as<int>();

        if (options_vars.count("initial-bound"))
            params.initial_bound = options_vars["initial-bound"].as<int>();

        if (options_vars.count("print-incumbents"))
            params.print_incumbents = true;

        if (options_vars.count("split-depth"))
            params.split_depth = options_vars["split-depth"].as<int>();

        if (options_vars.count("work-donation"))
            params.work_donation = true;

        if (options_vars.count("donate-when-idle"))
            params.donate_when_empty = false;

        if (options_vars.count("power"))
            params.power = options_vars["power"].as<int>();

        /* Read in the graph */
        auto graph = read_dimacs(options_vars["input-file"].as<std::string>());

        /* Work out manual-order, now that we know the graph size */
        if (std::get<2>(*algorithm)) {
            if (! options_vars.count("manual-order")) {
                std::cerr << "manual-order must be specified for this algorithm" << std::endl;
                return EXIT_FAILURE;
            }

            std::vector<std::string> tokens;
            boost::split(tokens, options_vars["manual-order"].as<std::string>(), boost::is_any_of(","));
            if (unsigned(tokens.size()) != unsigned(graph.size())) {
                std::cerr << "number of tokens specified for manual-order must be equal to the graph size" << std::endl;
                return EXIT_FAILURE;
            }

            for (auto & token : tokens) {
                auto p = token.find(':');
                if (std::string::npos == p) {
                    std::cerr << "manual-order tokens must be of the form vertex:bound" << std::endl;
                    return EXIT_FAILURE;
                }

                params.initial_order_and_bounds.emplace_back(std::stoi(token.substr(0, p)) - 1, std::stoi(token.substr(p + 1)));
            }
        }
        else if (options_vars.count("manual-order")) {
            std::cerr << "manual-order cannot be used for this algorithm" << std::endl;
            return EXIT_FAILURE;
        }


        /* Do the actual run. */
        bool aborted = false;
        auto result = std::get<1>(*algorithm)(
                graph,
                params,
                aborted,
                options_vars.count("timeout") ? options_vars["timeout"].as<int>() : 0);

        /* Stop the clock. */
        auto overall_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - params.start_time);

        /* Display the results. */
        std::cout << result.size << " " << result.nodes;
        if (aborted)
            std::cout << " aborted " << result.top_nodes_done;
        std::cout << std::endl;

        for (auto v : result.members)
            std::cout << v + 1 << " ";
        std::cout << std::endl;

        std::cout << overall_time.count();
        if (! result.times.empty()) {
            for (auto t : result.times)
                std::cout << " " << t.count();
        }
        std::cout << std::endl;

        if (params.work_donation)
            std::cout << result.donations << std::endl;

        if (options_vars.count("verify")) {
            for (auto & a : result.members)
                for (auto & b : result.members)
                    if (a != b && ! graph.adjacent(a, b)) {
                        std::cerr << "Oops! Not adjacent: " << a << " " << b << std::endl;
                        return EXIT_FAILURE;
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

