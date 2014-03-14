/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <boost/program_options.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <set>

namespace po = boost::program_options;

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                  "Display help information")
            ("format",      po::value<std::string>(), "Specify the format of the output")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("n",                  po::value<int>(),     "The number of vertices")
            ("p",                  po::value<double>(),  "The edge probability")
            ("s",                  po::value<int>(),     "The seed")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("n", 1)
            .add("p", 1)
            .add("s", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] n p s" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No n specified? Show a message and exit. */
        if (! options_vars.count("n") || ! options_vars.count("p") || ! options_vars.count("s")) {
            std::cout << "Usage: " << argv[0] << " [options] n p s" << std::endl;
            return EXIT_FAILURE;
        }

        int n = options_vars["n"].as<int>();
        double p = options_vars["p"].as<double>();
        int s = options_vars["s"].as<int>();

        std::mt19937 rand;
        rand.seed(s);
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        std::function<void (int, int)> output_function;

        if (! options_vars.count("format") || options_vars["format"].as<std::string>() == "dimacs") {
            std::cout << "p edge " << n << " 0" << std::endl;
            output_function = [] (int e, int f) { std::cout << "e " << e << " " << f << std::endl; };
        }
        else if (options_vars["format"].as<std::string>() == "pairs0") {
            std::cout << n << " 0" << std::endl;
            output_function = [] (int e, int f) { std::cout << e - 1 << " " << f - 1 << std::endl; };
        }
        else {
            std::cout << "Unknown format (try 'dimacs' or 'pairs0')" << std::endl;
            return EXIT_FAILURE;
        }

        for (int e = 1 ; e <= n ; ++e) {
            for (int f = e + 1 ; f <= n ; ++f) {
                if (dist(rand) <= p)
                    output_function(e, f);
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

