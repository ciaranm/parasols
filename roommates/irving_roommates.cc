/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <roommates/irving_roommates.hh>
#include <algorithm>

using namespace parasols;

namespace
{
    auto phase1_reduce(
            const RoommatesProblem & problem,
            std::vector<unsigned> & leftmost,
            std::vector<unsigned> & rightmost) -> bool
    {
        std::vector<bool> proposed_to((problem.size));

        for (unsigned person = 0 ; person != problem.size ; ++person) {
            unsigned proposer = person;
            unsigned next_choice, current;

            do {
                while (true) {
                    next_choice = problem.preferences[proposer][leftmost[proposer]];
                    current = problem.preferences[next_choice][rightmost[next_choice]];

                    /* has someone been rejected by everyone? */
                    if (next_choice == proposer)
                        return false;

                    /* proposer proposes to next_choice */
                    if (problem.rankings[next_choice][proposer] <= problem.rankings[next_choice][current])
                        break;

                    /* rejected */
                    ++leftmost[proposer];
                }

                rightmost[next_choice] = problem.rankings[next_choice][proposer];
                proposer = current;
            } while (proposed_to[next_choice]);

            proposed_to[next_choice] = true;
        }

        return true;
    }

    auto seek_cycle(
            const RoommatesProblem & problem,
            unsigned & first_in_cycle,
            unsigned & last_in_cycle,
            const unsigned first_unmatched,
            std::vector<bool> & tail,
            std::vector<unsigned> & cycle,
            std::vector<unsigned> & second,
            const std::vector<unsigned> & rightmost) -> void
    {
        unsigned person, posn_in_cycle;
        std::vector<bool> cycle_set;

        if (first_in_cycle > 0) {
            person = cycle[first_in_cycle - 1];
            posn_in_cycle = first_in_cycle - 1;
            cycle_set = tail;
        }
        else {
            person = first_unmatched;
            posn_in_cycle = 0;
            cycle_set.resize(problem.size);
        }

        do {
            cycle_set[person] = true;
            cycle[posn_in_cycle++] = person;
            unsigned pos_in_list = second[person];
            unsigned next_choice;

            do {
                next_choice = problem.preferences[person][pos_in_list++];
            } while (problem.rankings[next_choice][person] > rightmost[next_choice]);

            second[person] = pos_in_list - 1;
            person = problem.preferences[next_choice][rightmost[next_choice]];
        } while (! cycle_set[person]);

        last_in_cycle = posn_in_cycle - 1;
        tail = cycle_set;

        do {
            --posn_in_cycle;
            tail[cycle[posn_in_cycle]] = false;
        } while (cycle[posn_in_cycle] != person);

        first_in_cycle = posn_in_cycle;
    }

    auto phase2_reduce(
            const RoommatesProblem & problem,
            const unsigned first_in_cycle,
            const unsigned last_in_cycle,
            const std::vector<unsigned> & cycle,
            std::vector<unsigned> & second,
            std::vector<unsigned> & leftmost,
            std::vector<unsigned> & rightmost) -> bool
    {
        for (unsigned rank = first_in_cycle ; rank <= last_in_cycle ; ++rank) {
            unsigned proposer = cycle[rank];
            leftmost[proposer] = second[proposer];
            second[proposer] = leftmost[proposer] + 1;
            unsigned next_choice = problem.preferences[proposer][leftmost[proposer]];
            rightmost[next_choice] = problem.rankings[next_choice][proposer];
        }

        for (unsigned rank = first_in_cycle ; rank <= last_in_cycle ; ++rank) {
            unsigned proposer = cycle[rank];
            if (leftmost[proposer] > rightmost[proposer])
                return false;
        }

        return true;
    }
}

auto parasols::irving_roommates(const RoommatesProblem & problem, const RoommatesParams &) -> RoommatesResult
{
    RoommatesResult result;

    std::vector<unsigned> leftmost((problem.size + 1)), rightmost((problem.size + 1));

    std::fill(leftmost.begin(), leftmost.end(), 0);
    std::fill(rightmost.begin(), rightmost.end(), problem.size - 1);

    if (phase1_reduce(problem, leftmost, rightmost)) {
        result.success = true;

        std::vector<unsigned> second((problem.size)), cycle((problem.size));
        std::vector<bool> tail((problem.size));
        unsigned first_in_cycle = 0, last_in_cycle;

        for (unsigned person = 0 ; person != problem.size ; ++person)
            second[person] = leftmost[person] + 1;

        /* find the first matching (a, b) such that b isn't matched to a */
        for (unsigned first_unmatched = 0 ; first_unmatched < problem.size ; ) {
            if (leftmost[first_unmatched] == rightmost[first_unmatched]) {
                ++first_unmatched;
                first_in_cycle = 0;
                continue;
            }

            /* look for a cycle, and reduce it */
            seek_cycle(problem, first_in_cycle, last_in_cycle, first_unmatched, tail, cycle, second, rightmost);

            if (! phase2_reduce(problem, first_in_cycle, last_in_cycle, cycle, second, leftmost, rightmost)) {
                result.success = false;
                break;
            }
        }
    }

    if (result.success) {
        for (unsigned person = 0 ; person != problem.size ; ++person)
            result.members.insert(std::make_pair(person, problem.preferences[person][leftmost[person]]));
    }

    return result;
}

