/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <boost/program_options.hpp>

#include <graph/graph.hh>
#include <graph/product.hh>
#include <graph/file_formats.hh>

#include <iostream>
#include <exception>
#include <cstdlib>
#include <set>

using namespace parasols;
namespace po = boost::program_options;

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                         "Display help information")
            ("format",             po::value<std::string>(), "Specify the format of the input")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("graph1", "First input graph (DIMACS format, unless --format is specified).")
            ("graph2", "Second input graph (DIMACS format, unless --format is specified).")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
            .add("graph1", 1)
            .add("graph2", 1)
            ;

        po::variables_map options_vars;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(), options_vars);
        po::notify(options_vars);

        /* --help? Show a message, and exit. */
        if (options_vars.count("help")) {
            std::cout << "Usage: " << argv[0] << " [options] graph1 graph2" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No n specified? Show a message and exit. */
        if (! options_vars.count("graph1") || ! options_vars.count("graph2")) {
            std::cout << "Usage: " << argv[0] << " [options] graph1 graph2" << std::endl;
            return EXIT_FAILURE;
        }

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
        auto graph1 = std::get<1>(*format)(options_vars["graph1"].as<std::string>());
        auto graph2 = std::get<1>(*format)(options_vars["graph2"].as<std::string>());

        auto product = modular_product(graph1, graph2);

        std::cout << "p edge " << product.size() << " 0" << std::endl;
        for (int e = 0 ; e < product.size() ; ++e) {
            for (int f = e + 1 ; f < product.size() ; ++f) {
                if (product.adjacent(e, f))
                    std::cout << "e " << (e + 1) << " " << (f + 1) << std::endl;
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


