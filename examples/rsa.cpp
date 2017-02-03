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

#include <iostream>
#include "checkcall.hpp"
#include "guard.hpp"

#include "boost/format.hpp"

extern "C" {
#include "openssl/bn.h"
#include "openssl/err.h"
#include "openssl/rand.h"
#include "openssl/rsa.h"
}

constexpr int INDENT{1};

int rsaKeygenCWay() {
    int error = 0;
    if (!RAND_status()) {
        std::cerr << "Not enough entropy\n";
        return 1;
    }
    RSA *rsa = RSA_new();
    if (nullptr == rsa) {
        std::cerr << ERR_error_string(ERR_get_error(), nullptr) << "\n";
        return 1;
    }
    BIGNUM *exponent = BN_new();
    if (nullptr == exponent) {
        error = 1;
        std::cerr << ERR_error_string(ERR_get_error(), nullptr) << "\n";
        goto out_rsa;
    }

    if (!BN_set_word(exponent, 65537)) {
        error = 1;
        std::cerr << ERR_error_string(ERR_get_error(), nullptr) << "\n";
        goto out;
    }

    if (!RSA_generate_key_ex(rsa, 2048, exponent, nullptr)) {
        error = 1;
        std::cerr << ERR_error_string(ERR_get_error(), nullptr) << "\n";
        goto out;
    }

    if (!RSA_print_fp(stdout, rsa, INDENT)) {
        error = 1;
        std::cerr << ERR_error_string(ERR_get_error(), nullptr) << "\n";
        goto out;
    }
out:
    BN_free(exponent);
out_rsa:
    RSA_free(rsa);
    return error;
}

struct RSADeleter {
    void operator()(RSA *rsa) const { RSA_free(rsa); }
};

struct BNDeleter {
    void operator()(BIGNUM *bn) const { BN_free(bn); }
};

using RSAUniquePtr = std::unique_ptr<RSA, RSADeleter>;
using BNUniquePtr = std::unique_ptr<BIGNUM, BNDeleter>;

int rsaKeygenUniquePtr() {
    if (!RAND_status()) {
        throw std::runtime_error((boost::format("%s: %d") %
                                  ERR_error_string(ERR_get_error(), nullptr) % ERR_get_error())
                                         .str());
    }
    RSAUniquePtr rsa{RSA_new()};
    if (nullptr == rsa) {
        throw std::runtime_error((boost::format("%s: %d") %
                                  ERR_error_string(ERR_get_error(), nullptr) % ERR_get_error())
                                         .str());
    }

    BNUniquePtr exponent{BN_new()};
    if (nullptr == exponent) {
        throw std::runtime_error((boost::format("%s: %d") %
                                  ERR_error_string(ERR_get_error(), nullptr) % ERR_get_error())
                                         .str());
    }

    if (!BN_set_word(exponent.get(), 65537)) {
        throw std::runtime_error((boost::format("%s: %d") %
                                  ERR_error_string(ERR_get_error(), nullptr) % ERR_get_error())
                                         .str());
    }

    if (!RSA_generate_key_ex(rsa.get(), 2048, exponent.get(), nullptr)) {
        throw std::runtime_error((boost::format("%s: %d") %
                                  ERR_error_string(ERR_get_error(), nullptr) % ERR_get_error())
                                         .str());
    }

    if (!RSA_print_fp(stdout, rsa.get(), INDENT)) {
        throw std::runtime_error((boost::format("%s: %d") %
                                  ERR_error_string(ERR_get_error(), nullptr) % ERR_get_error())
                                         .str());
    }
    return 0;
}

using RSAGuard = cwrap::Guard<RSA *, RSADeleter>;
using BNGuard = cwrap::Guard<BIGNUM *, BNDeleter>;

struct OpenSSLErrorPolicy {
    template <class Rv>
    static void handleError(const Rv &);
};

template <class Rv>
void OpenSSLErrorPolicy::handleError(const Rv &) {
    throw std::runtime_error(
            (boost::format("%s: %d") % ERR_error_string(ERR_get_error(), nullptr) % ERR_get_error())
                    .str());
}

// openssl-functions use a non-zero return code to indicate error
using ct = cwrap::CallCheckContext<cwrap::IsNotZeroReturnCheckPolicy, OpenSSLErrorPolicy>;
using ct_ptr = cwrap::CallCheckContext<cwrap::IsNotNullptrReturnCheckPolicy, OpenSSLErrorPolicy>;

int rsaKeygenCWrapWay() {
    ct::callChecked(RAND_status);
    RSAGuard rsa{ct_ptr::callChecked(RSA_new)};
    BNGuard exponent{ct_ptr::callChecked(BN_new)};
    ct::callChecked(BN_set_word, exponent.get(), 65537);
    ct::callChecked(RSA_generate_key_ex, rsa.get(), 2048, exponent.get(), nullptr);
    ct::callChecked(RSA_print_fp, stdout, rsa.get(), INDENT);
    return 0;
}

int main() { return rsaKeygenCWrapWay(); }
