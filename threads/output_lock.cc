/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <threads/output_lock.hh>

using namespace parasols;

namespace
{
    static std::mutex output_mutex;
}

auto parasols::lock_output() -> OutputLock
{
    return OutputLock{ std::unique_lock<std::mutex>{ output_mutex } };
}

std::ostream & parasols::operator<< (std::ostream & s, const OutputLock &)
{
    return s;
}

