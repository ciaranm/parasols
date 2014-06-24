/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_BICLIQUE_ALGORITHMS_HH
#define PARASOLS_GUARD_MAX_BICLIQUE_ALGORITHMS_HH 1

#include <max_biclique/naive_max_biclique.hh>
#include <max_biclique/ccd_max_biclique.hh>
#include <max_biclique/dccd_max_biclique.hh>
#include <max_biclique/cpo_max_biclique.hh>

#include <utility>

namespace parasols
{
    auto max_biclique_algorithms = {
        std::make_pair( std::string{ "naiven" },      naive_max_biclique<BicliqueSymmetryRemoval::None> ),
        std::make_pair( std::string{ "naiver" },      naive_max_biclique<BicliqueSymmetryRemoval::Remove> ),

        std::make_pair( std::string{ "ccdn" },        ccd_max_biclique<BicliqueSymmetryRemoval::None> ),
        std::make_pair( std::string{ "ccdr" },        ccd_max_biclique<BicliqueSymmetryRemoval::Remove> ),

        std::make_pair( std::string{ "dccdn" },       dccd_max_biclique<BicliqueSymmetryRemoval::None>),
        std::make_pair( std::string{ "dccdr" },       dccd_max_biclique<BicliqueSymmetryRemoval::Remove>),

        std::make_pair( std::string{ "cponn" },       cpo_max_biclique<CCOPermutations::None, BicliqueSymmetryRemoval::None> ),
        std::make_pair( std::string{ "cponr" },       cpo_max_biclique<CCOPermutations::None, BicliqueSymmetryRemoval::Remove> ),
        std::make_pair( std::string{ "cpons" },       cpo_max_biclique<CCOPermutations::None, BicliqueSymmetryRemoval::Skip> ),
        std::make_pair( std::string{ "cpodn" },       cpo_max_biclique<CCOPermutations::Defer1, BicliqueSymmetryRemoval::None> ),
        std::make_pair( std::string{ "cpodr" },       cpo_max_biclique<CCOPermutations::Defer1, BicliqueSymmetryRemoval::Remove> ),
        std::make_pair( std::string{ "cposn" },       cpo_max_biclique<CCOPermutations::Sort, BicliqueSymmetryRemoval::None> ),
        std::make_pair( std::string{ "cposr" },       cpo_max_biclique<CCOPermutations::Sort, BicliqueSymmetryRemoval::Remove> )
    };
}

#endif
