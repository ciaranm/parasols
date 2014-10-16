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
            ("input-file", po::value<std::vector<std::string> >(),
                           "Specify the input files (DIMACS format, unless --format is specified).")
            ;

        all_options.add(display_options);

        po::positional_options_description positional_options;
        positional_options
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
            std::cout << "Usage: " << argv[0] << " [options] graph..." << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No graphs specified? Show a message and exit. */
        if (options_vars.count("input-file") < 1) {
            std::cout << "Usage: " << argv[0] << " [options] graph..." << std::endl;
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

        Graph result(0, true);

        /* Read in the graphs */
        auto input_files = options_vars["input-file"].as<std::vector<std::string> >();
        for (auto & input_file : input_files) {
            auto graph = std::get<1>(*format)(input_file);

            if (0 == result.size())
                result.resize(graph.size());

            if (graph.size() != result.size()) {
                std::cerr << "Graph size mismatch" << std::endl;
                return EXIT_FAILURE;
            }

            for (int i = 0 ; i < graph.size() ; ++i)
                for (int j = 0 ; j < graph.size() ; ++j)
                    if (graph.adjacent(i, j))
                        result.add_edge(i, j);
        }

        std::cout << "p edge " << result.size() << " 0" << std::endl;
        for (int e = 0 ; e < result.size() ; ++e) {
            for (int f = e + 1 ; f < result.size() ; ++f) {
                if (result.adjacent(e, f))
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


