/*   Copyright 2016,2017 Marcus Gelderie
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
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
