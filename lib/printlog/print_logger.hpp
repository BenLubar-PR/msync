#ifndef _PRINTLOG_HPP_
#define _PRINTLOG_HPP_

#include <fstream>
#include <iostream>

#include "../options/options.hpp"

using std::cout;
using std::ofstream;

enum class logtype
{
    normal,
    verbose,
    fileonly
};

template <logtype isverbose = logtype::normal>
struct print_logger
{
    print_logger() : logfile("msync.log") {}

    template <typename T>
    print_logger& operator<<(const T& towrite)
    {
        if constexpr (isverbose == logtype::verbose)
        {
            if (options.verbose)
                cout << towrite;
        }
        else if constexpr (isverbose != logtype::fileonly)
        {
            cout << towrite;
        }

        logfile << towrite;
        return *this;
    }

private:
    ofstream logfile;
};
#endif