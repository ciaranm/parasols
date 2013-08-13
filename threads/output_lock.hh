/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_THREADS_OUTPUT_LOCK_HH
#define PARASOLS_GUARD_THREADS_OUTPUT_LOCK_HH 1

#include <ostream>
#include <mutex>

namespace parasols
{
    struct OutputLock
    {
        std::unique_lock<std::mutex> guard;
    };

    // Usage: std::cerr << lock_output() << etc etc
    auto lock_output() -> OutputLock;

    std::ostream & operator<< (std::ostream &, const OutputLock &);
}

#endif
