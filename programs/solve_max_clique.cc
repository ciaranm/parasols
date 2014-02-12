/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <solver/solver.hh>

#include <graph/graph.hh>
#include <graph/dimacs.hh>
#include <graph/pairs.hh>
#include <graph/net.hh>
#include <graph/power.hh>
#include <graph/complement.hh>
#include <graph/is_clique.hh>
#include <graph/is_club.hh>
#include <graph/degree_sort.hh>
#include <graph/min_width_sort.hh>

#include <max_clique/naive_max_clique.hh>
#include <max_clique/bmcsa_max_clique.hh>
#include <max_clique/tbmcsa_max_clique.hh>
#include <max_clique/dbmcsa_max_clique.hh>
#include <max_clique/cco_max_clique.hh>

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
                    if (params.power > 1)
                        return func(power(graph, params.power), params);
                    else
                        return func(graph, params);
                });
    }
}

auto main(int argc, char * argv[]) -> int
{
    using namespace std::placeholders;

    auto algorithms = {
        std::make_tuple( std::string{ "naive" },     run_with_power(naive_max_clique) ),

        std::make_tuple( std::string{ "bmcsa" },     run_with_power(bmcsa_max_clique) ),

        std::make_tuple( std::string{ "ccon" },      run_with_power(cco_max_clique<CCOPermutations::None>) ),
        std::make_tuple( std::string{ "ccod" },      run_with_power(cco_max_clique<CCOPermutations::Defer1>) ),
        std::make_tuple( std::string{ "ccos" },      run_with_power(cco_max_clique<CCOPermutations::Sort>) ),

        std::make_tuple( std::string{ "tbmcsa" },    run_with_power(tbmcsa_max_clique) ),

        std::make_tuple( std::string{ "dbmcsa" },    run_with_power(dbmcsa_max_clique) )
    };

    auto orders = {
        std::make_tuple( std::string{ "deg" },       std::bind(degree_sort, _1, _2, false) ),
        std::make_tuple( std::string{ "ex" },        std::bind(exdegree_sort, _1, _2, false) ),
        std::make_tuple( std::string{ "dynex" },     std::bind(dynexdegree_sort, _1, _2, false) ),
        std::make_tuple( std::string{ "mw" },        std::bind(min_width_sort, _1, _2, false) )
    };

    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                 "Display help information")
            ("threads",            po::value<int>(), "Number of threads to use (where relevant)")
            ("stop-after-finding", po::value<int>(), "Stop after finding a clique of this size")
            ("initial-bound",      po::value<int>(), "Specify an initial bound")
            ("enumerate",                            "Enumerate solutions (use with bmcsa1 --initial-bound=omega-1 --print-incumbents)")
            ("print-incumbents",                     "Print new incumbents as they are found")
            ("split-depth",        po::value<int>(), "Specify the depth at which to perform splitting (where relevant)")
            ("work-donation",                        "Enable work donation (where relevant)")
            ("timeout",            po::value<int>(), "Abort after this many seconds")
            ("complement",                           "Take the complement of the graph (to solve independent set)")
            ("power",              po::value<int>(), "Raise the graph to this power (to solve s-clique)")
            ("verify",                               "Verify that we have found a valid result (for sanity checking changes)")
            ("check-club",                           "Check whether our s-clique is also an s-club")
            ("pairs",                                "Input is in pairs format, not DIMACS")
            ("net",                                  "Input is in net format, not DIMACS")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("algorithm",  "Specify which algorithm to use")
            ("order",      "Specify the initial vertex order")
            ("input-file", po::value<std::vector<std::string> >(),
                           "Specify the input file (DIMACS format, unless --pairs or --net is specified). May be specified multiple times.")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("algorithm", 1)
            .add("order", 1)
            .add("input-file", -1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm order file[...]" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (! options_vars.count("algorithm") || options_vars.count("input-file") < 1) {
            std::cout << "Usage: " << argv[0] << " [options] algorithm order file[...]" << std::endl;
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

        /* For each input file... */
        auto input_files = options_vars["input-file"].as<std::vector<std::string> >();
        bool first = true;
        for (auto & input_file : input_files) {
            if (first)
                first = false;
            else
                std::cout << "--" << std::endl;

            /* Figure out what our options should be. */
            MaxCliqueParams params;

            params.order_function = order_function;

            if (options_vars.count("threads"))
                params.n_threads = options_vars["threads"].as<int>();
            else
                params.n_threads = std::thread::hardware_concurrency();

            if (options_vars.count("stop-after-finding"))
                params.stop_after_finding = options_vars["stop-after-finding"].as<int>();

            if (options_vars.count("initial-bound"))
                params.initial_bound = options_vars["initial-bound"].as<int>();

            if (options_vars.count("enumerate"))
                params.enumerate = true;

            if (options_vars.count("print-incumbents"))
                params.print_incumbents = true;

            if (options_vars.count("check-club"))
                params.check_clubs = true;

            if (options_vars.count("split-depth"))
                params.split_depth = options_vars["split-depth"].as<int>();

            if (options_vars.count("work-donation"))
                params.work_donation = true;

            if (options_vars.count("power"))
                params.power = options_vars["power"].as<int>();

            /* Read in the graph */
            auto graph = options_vars.count("pairs") ?
                read_pairs(input_file) : options_vars.count("net") ? read_net(input_file) : read_dimacs(input_file);

            if (options_vars.count("complement")) {
                graph = complement(graph); // don't time this
                params.complement = true;
            }

            params.original_graph = &graph;

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

            if (options_vars.count("enumerate")) {
                std::cout << " " << result.result_count;
                if (options_vars.count("check-club"))
                    std::cout << " " << result.result_club_count;
            }

            if (aborted)
                std::cout << " aborted";

            std::cout << std::endl;

            /* Members, and whether it's a club. */
            for (auto v : result.members)
                std::cout << graph.vertex_name(v) << " ";

            if (options_vars.count("check-club")) {
                if (is_club(graph, params.power, result.members))
                    std::cout << "(club)" << std::endl;
                else
                    std::cout << "(not club)" << std::endl;
            }

            std::cout << std::endl;

            /* Times */
            std::cout << overall_time.count();
            if (! result.times.empty()) {
                for (auto t : result.times)
                    std::cout << " " << t.count();
            }
            std::cout << std::endl;

            /* Donation */
            if (params.work_donation)
                std::cout << result.donations << std::endl;

            if (options_vars.count("verify")) {
                if (params.power > 1) {
                    if (! is_clique(power(graph, params.power), result.members)) {
                        std::cerr << "Oops! not a clique" << std::endl;
                        return EXIT_FAILURE;
                    }
                }
                else {
                    if (! is_clique(graph, result.members)) {
                        std::cerr << "Oops! not a clique" << std::endl;
                        return EXIT_FAILURE;
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

