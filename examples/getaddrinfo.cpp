#include <iostream>

#include "cwrap.hpp"

extern "C" {
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
}

struct GetAddrInfoErrorPolicy {
    template <class Rv>
    static void handleError(const Rv &rv);
};

template <class Rv>
void GetAddrInfoErrorPolicy::handleError(const Rv &rv) {
    throw std::runtime_error(gai_strerror(rv));
}

/**
 * getprotobynumber() returns nullptr on error.
 *
 * This ErrorPolicy modifies the return value
 * to return a dummy structure on error. This allows
 * code to just assume the return value is not null.
 *
 */
struct GetProtoByNumberErrorPolicy {
private:
    static char _name[1];
    static struct protoent _protoent;

public:
    template <class Rv>
    static auto handleError(const Rv &) {
        return &_protoent;
    }

    /**
     * If the return value is ok, we just hand it through.
     *
     * Note that this function is implicitly inline (because
     * it is defined inside the class body).
     */
    template <class Rv>
    static auto handleOk(const Rv &rv) {
        return rv;
    }
};
char GetProtoByNumberErrorPolicy::_name[1] = "";

struct protoent GetProtoByNumberErrorPolicy::_protoent = {
        .p_name = _name, .p_aliases = nullptr, .p_proto = 0,
};

using ct = cwrap::CallCheckContext<cwrap::IsZeroReturnCheckPolicy, GetAddrInfoErrorPolicy>;
using nullsafe =
        cwrap::CallCheckContext<cwrap::IsNotNullptrReturnCheckPolicy, GetProtoByNumberErrorPolicy>;

int cwrapWay(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Not enough arguments.\n"
                  << "usage: getaddrinfo <host>\n";
        return -1;
    }
    const char *node = argv[1];

    cwrap::Guard<struct addrinfo *> addrinfoList{[](auto *ptr) {
        std::cout << "Freeing list ...\n";
        freeaddrinfo(ptr);
    }};

    struct addrinfo hint;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0;  // any protocol
    hint.ai_flags = 0;

    /*
     * Because we have all the memory management and error handling code elsewhere,
     * the code below focuses only on the actual code-path that we are interested in.
     * This yields more readable code.
     */
    ct::callChecked(getaddrinfo, node, nullptr, &hint, &addrinfoList.get());
    for (auto listPtr = addrinfoList.get(); listPtr != nullptr; listPtr = listPtr->ai_next) {
        auto *addressPtr = reinterpret_cast<struct sockaddr_in *>(listPtr->ai_addr);
        std::cout << inet_ntoa(addressPtr->sin_addr) << "\t"
                  // note how we can now just assume the return value is non-null
                  << nullsafe::callChecked(getprotobynumber, listPtr->ai_protocol)->p_name << "\n";
    }
    return 0;
}

/**
 * For comparison: This is how to handle this call without CWrap (or similar custom code).
 *
 * Note how cumbersome the unique_ptr makes the error handling.
 *
 * Note also the mix of error handling code and primary business logic.
 */
int plainCpp11(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Not enough arguments.\n"
                  << "usage: getaddrinfo <host>\n";
        return -1;
    }
    const char *node = argv[1];

    //note how we now have to allocate and delete memory manually, just to store a ptr
    auto listDeleter = [](auto *ptr) {
        std::cout << "Freeing list ...\n";
        freeaddrinfo(*ptr);
        delete ptr;
    };
    auto addrinfoList = std::unique_ptr<struct addrinfo *, decltype(listDeleter)>(
            new struct addrinfo *, listDeleter);

    struct addrinfo hint;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0;  // any protocol
    hint.ai_flags = 0;

    int errorcode = getaddrinfo(node, nullptr, &hint, addrinfoList.get());
    if (errorcode) {
        throw std::runtime_error(gai_strerror(errorcode));
    }

    for (auto listPtr = *addrinfoList; listPtr != nullptr; listPtr = listPtr->ai_next) {
        auto *addressPtr = reinterpret_cast<struct sockaddr_in *>(listPtr->ai_addr);
        std::cout << inet_ntoa(addressPtr->sin_addr) << "\t";

        auto protoNamePtr = getprotobynumber(listPtr->ai_protocol);
        if (protoNamePtr) {
            std::cout << protoNamePtr->p_name;
        }

        std::cout << "\n";
    }
    return 0;
}

int main(int argc, char **argv) { return cwrapWay(argc, argv); }
