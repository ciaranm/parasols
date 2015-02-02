/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/graph.hh>
#include <graph/file_formats.hh>
#include <graph/power.hh>
#include <graph/complement.hh>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <exception>
#include <cstdlib>

using namespace parasols;
namespace po = boost::program_options;

auto main(int argc, char * argv[]) -> int
{
    try {
        po::options_description display_options{ "Program options" };
        display_options.add_options()
            ("help",                                 "Display help information")
            ("complement",                           "Take the complement of the graph")
            ("power",              po::value<int>(), "Raise the graph to this power")
            ("format",             po::value<std::string>(), "Specify the format of the input")
            ;

        po::options_description all_options{ "All options" };
        all_options.add_options()
            ("input-file", po::value<std::vector<std::string> >(),
                           "Specify the input file (DIMACS format, unless --format is specified). May be specified multiple times.")
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
            std::cout << "Usage: " << argv[0] << " [options] file[...]" << std::endl;
            std::cout << std::endl;
            std::cout << display_options << std::endl;
            return EXIT_SUCCESS;
        }

        /* No algorithm or no input file specified? Show a message and exit. */
        if (options_vars.count("input-file") < 1) {
            std::cout << "Usage: " << argv[0] << " [options] file[...]" << std::endl;
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

            /* Read in the graph */
            auto graph = std::get<1>(*format)(input_file, GraphOptions::AllowLoops);

            if (options_vars.count("complement"))
                graph = complement(graph);

            if (options_vars.count("power"))
                graph = power(graph, options_vars["power"].as<int>());

            unsigned edges = 0;
            unsigned loops = 0;
            unsigned max_deg = 0;
            unsigned mean_deg = 0;
            for (int i = 0 ; i < graph.size() ; ++i) {
                if (graph.adjacent(i, i))
                    ++loops;

                mean_deg += graph.degree(i);
                max_deg = std::max<unsigned>(max_deg, graph.degree(i));

                for (int j = 0 ; j <= i ; ++j)
                    if (graph.adjacent(i, j))
                        ++edges;
            }

            std::cout << graph.size() << " " << edges << " " << loops << " " <<
                ((0.0 + mean_deg) / (0.0 + graph.size())) << " " << max_deg << " "
                 << ((0.0 + 2 * edges) / (graph.size() * (graph.size() - 1))) << std::endl;
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

