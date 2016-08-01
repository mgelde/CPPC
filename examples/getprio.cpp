/*
 *  CWrap - A library to use C-code in C++.
 *  Copyright (C) 2016  Marcus Gelderie
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include <cerrno>
#include <cstring>
#include <iostream>

#include "checkcall.hpp"
#include "guard.hpp"

extern "C" {
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
}

/*
 * In Linux, getpriority(int, id_t) can ligitimately return -1.
 * For this reason, a caller has to check errno. But since errno
 * can be set by other calls as well, it must first be cleared.
 *
 */

//uses some C++, but is essentially C...
int getPrioUsingPlainOldC(pid_t pid) {
    using namespace std;
    errno = 0;
    int prio = getpriority(PRIO_PROCESS, pid);
    if (prio == -1 && errno != 0) {
        int savedErrno = errno;
        cerr << strerror(savedErrno) << "\n";
        return savedErrno;
    }
    cout << "PID is " << pid << " and NICE is " << prio << "\n";
    return 0;
}

//for CWrap, we first need to define a ReturnCheckPolicy

struct GetPrioReturnCheckPolicy {
    static inline bool returnValueIsOk(int prio) {
        std::cout << "Checking if prio '" << prio << "' is ok...\n";
        return prio != -1 || errno == 0;
    }

    static inline void preCall() {
        std::cout << "Calling 'preCall'...\n";
        errno = 0;
    }
};

using ct = cwrap::CallCheckContext<GetPrioReturnCheckPolicy, cwrap::ErrnoErrorPolicy>;

int getPrioUsingCWrap(pid_t pid) {
    int prio = ct::callChecked(getpriority, PRIO_PROCESS, pid);
    std::cout << "PID is " << pid << " and NICE is " << prio << "\n";
    return 0;
}

int main() {
    pid_t pid = getpid();
    return getPrioUsingCWrap(pid);
}
