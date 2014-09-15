/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_CCO_CCO_MIXIN_HH
#define PARASOLS_GUARD_CCO_CCO_MIXIN_HH 1

#include <cco/cco.hh>
#include <graph/bit_graph.hh>

#include <list>
#include <vector>

namespace parasols
{
    /**
     * Turn a CCOPermutations into a type.
     */
    template <CCOPermutations perm_>
    struct SelectColourClassOrderOverload
    {
    };

    template <unsigned size_, typename VertexType_, typename ActualType_, bool inverse_>
    struct CCOMixin
    {
        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::None> &,
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds,
                int = 0) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            VertexType_ colour = 0;        // current colour
            VertexType_ i = 0;             // position in p_bounds

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<size_> q = p_left;

                // while we can still give something this colour
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    if (inverse_)
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row(v, q);
                    else
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row_complement(v, q);

                    // record in result
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                }
            }
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::Defer1> &,
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds,
                int = 0) -> void
        {
            FixedBitSet<size_> p_left = p; // not coloured yet
            VertexType_ colour = 0;        // current colour
            VertexType_ i = 0;             // position in p_bounds

            VertexType_ d = 0;             // number deferred
            std::array<VertexType_, size_ * bits_per_word> defer;

            // while we've things left to colour
            while (! p_left.empty()) {
                // next colour
                ++colour;
                // things that can still be given this colour
                FixedBitSet<size_> q = p_left;

                // while we can still give something this colour
                unsigned number_with_this_colour = 0;
                while (! q.empty()) {
                    // first thing we can colour
                    int v = q.first_set_bit();
                    p_left.unset(v);
                    q.unset(v);

                    // can't give anything adjacent to this the same colour
                    if (inverse_)
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row(v, q);
                    else
                        static_cast<ActualType_ *>(this)->graph.intersect_with_row_complement(v, q);

                    // record in result
                    p_bounds[i] = colour;
                    p_order[i] = v;
                    ++i;
                    ++number_with_this_colour;
                }

                if (1 == number_with_this_colour) {
                    --i;
                    --colour;
                    defer[d++] = p_order[i];
                }
            }

            for (VertexType_ n = 0 ; n < d ; ++n) {
                ++colour;
                p_order[i] = defer[n];
                p_bounds[i] = colour;
                i++;
            }
        }

        auto colour_class_order_with_repair(
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds,
                int delta,
                bool selective,
                bool do_defer
                ) -> void
        {
            static thread_local std::vector<std::pair<int, std::array<VertexType_, size_ * bits_per_word> > > colour_classes;

            FixedBitSet<size_> p_left = p; // not coloured yet
            int colour_classes_end = 0;

            while (! p_left.empty()) {
                int v = p_left.first_set_bit();
                p_left.unset(v);

                bool coloured = false;
                for (int colour_class = 0 ; colour_class != colour_classes_end ; ++colour_class) {
                    bool conflict = false;

                    for (int vertex_pos = 0 ; vertex_pos != colour_classes[colour_class].first ; ++vertex_pos)
                        if (static_cast<ActualType_ *>(this)->graph.adjacent(v, colour_classes[colour_class].second[vertex_pos])) {
                            conflict = true;
                            break;
                        }

                    if (! conflict) {
                        coloured = true;
                        colour_classes[colour_class].second[colour_classes[colour_class].first++] = v;
                        break;
                    }
                }

                if (! coloured) {
                    bool repaired = false;

                    if ((! selective) || (colour_classes_end >= delta)) {
                        for (int colour_class = 0 ; colour_class < colour_classes_end - 1 ; ++colour_class) {
                            int n_conflicts = 0;
                            int vertex_to_move = -1, vertex_to_move_pos = -1;
                            for (int vertex_pos = 0 ; vertex_pos != colour_classes[colour_class].first ; ++vertex_pos)
                                if (static_cast<ActualType_ *>(this)->graph.adjacent(v, colour_classes[colour_class].second[vertex_pos])) {
                                    vertex_to_move = colour_classes[colour_class].second[vertex_pos];
                                    vertex_to_move_pos = vertex_pos;
                                    if (++n_conflicts > 1)
                                        break;
                                }

                            if (1 == n_conflicts) {
                                for (int new_colour_class = colour_class + 1 ; new_colour_class < colour_classes_end ; ++new_colour_class) {
                                    bool conflict = false;
                                    for (int vertex_pos = 0 ; vertex_pos != colour_classes[new_colour_class].first ; ++vertex_pos)
                                        if (static_cast<ActualType_ *>(this)->graph.adjacent(vertex_to_move, colour_classes[new_colour_class].second[vertex_pos])) {
                                            conflict = true;
                                            break;
                                        }

                                    if (! conflict) {
                                        repaired = true;
                                        colour_classes[new_colour_class].second[colour_classes[new_colour_class].first++] = vertex_to_move;
                                        for (int x = vertex_to_move_pos ; x < colour_classes[colour_class].first - 1 ; ++x)
                                            colour_classes[colour_class].second[x] = colour_classes[colour_class].second[x + 1];
                                        colour_classes[colour_class].second[colour_classes[colour_class].first - 1] = v;
                                        break;
                                    }
                                }
                            }

                            if (repaired)
                                break;
                        }
                    }

                    if (! repaired) {
                        if (colour_classes.size() < unsigned(colour_classes_end + 1))
                            colour_classes.resize(colour_classes_end + 1);

                        colour_classes[colour_classes_end].first = 1;
                        colour_classes[colour_classes_end].second[0] = v;
                        ++colour_classes_end;
                    }
                }
            }

            VertexType_ colour = 0;        // current colour
            VertexType_ i = 0;             // position in p_bounds
            VertexType_ d = 0;             // number deferred
            std::array<VertexType_, size_ * bits_per_word> defer;
            for (int colour_class = 0 ; colour_class != colour_classes_end ; ++colour_class) {
                if (do_defer && 1 == colour_classes[colour_class].first)
                    defer[d++] = colour_classes[colour_class].second[0];
                else {
                    ++colour;
                    for (int vertex_pos = 0 ; vertex_pos != colour_classes[colour_class].first ; ++vertex_pos) {
                        p_bounds[i] = colour;
                        p_order[i] = colour_classes[colour_class].second[vertex_pos];
                        ++i;
                    }
                }
            }

            if (do_defer)
                for (VertexType_ n = 0 ; n < d ; ++n) {
                    ++colour;
                    p_order[i] = defer[n];
                    p_bounds[i] = colour;
                    i++;
                }
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::RepairAll> &,
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds,
                int delta = 0) -> void
        {
            colour_class_order_with_repair(p, p_order, p_bounds, delta, false, false);
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::RepairAllDefer1> &,
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds,
                int delta = 0) -> void
        {
            colour_class_order_with_repair(p, p_order, p_bounds, delta, false, true);
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::RepairSelected> &,
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds,
                int delta
                ) -> void
        {
            colour_class_order_with_repair(p, p_order, p_bounds, delta, true, false);
        }

        auto colour_class_order(
                const SelectColourClassOrderOverload<CCOPermutations::RepairSelectedDefer1> &,
                const FixedBitSet<size_> & p,
                std::array<VertexType_, size_ * bits_per_word> & p_order,
                std::array<VertexType_, size_ * bits_per_word> & p_bounds,
                int delta
                ) -> void
        {
            colour_class_order_with_repair(p, p_order, p_bounds, delta, true, true);
        }
    };

    template <template <CCOPermutations, unsigned, typename VertexType_> class WhichCCO_, CCOPermutations perm_>
    struct ApplyPerm
    {
        template <unsigned size_, typename VertexType_> using Type = WhichCCO_<perm_, size_, VertexType_>;
    };
}

#endif
