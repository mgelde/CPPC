[![Build Status](https://travis-ci.org/mgelde/CWrap.svg?branch=master)](https://travis-ci.org/mgelde/CWrap)

CWrap - Wrap C-code easily.
=====

Introduction
--------

CWrap is a header-only library designed to more easily use C-code from C++. It provides helper classes and methods to bring type and memory safety to C APIs. It also helps to avoid boilerplate code that typically stems from error handling in C.

Currently, CWrap provides the following features:  
* `checkcall`
A set of functions and functors to allow for easier, more readable error handling, while removing code duplication both in the binary and in the source file.
* `Guard`
A class to help with resource management. It is similar to a smart-pointer, but more geared toward the various C API patterns out there.

Functionality
-----

### checkcall

The `checkcall.hpp` header provides functions and functors to wrap C function-calls and transparently manage error handling. This can be used to transparently, and
consistently, verify the success of function calls, and to take appropriate action if the verification fais.

For example, consider the following code:
```cpp
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
```
This is how you create a new RSA context in [Open SSL](https://www.openssl.org/). Note that you need to do a lot of error handling. This includes some library-specific way of getting an indication of what actually went wrong.

Here's how to achieve the same using CWrap (well actually, the code below throws on error instead of logging to stderr...; but you can specify that with a policy via a template parameter!):

```cpp
    // openssl-functions use a non-zero return code to indicate error
    using ct = cwrap::CallCheckContext<cwrap::IsNotZeroReturnCheckPolicy, OpenSSLErrorPolicy>;
    //functions returning pointers return nullptr on error
    using ct_ptr = cwrap::CallCheckContext<cwrap::IsNotNullptrReturnCheckPolicy, OpenSSLErrorPolicy>;

    RSA *rsa = ct_ptr::callChecked(RSA_new);
    BIGNUM *exponent = ct_ptr::callChecked(BN_new);
    ct::callChecked(BN_set_word, exponent.get(), 65537);
    ct::callChecked(RSA_generate_key_ex, rsa.get(), 2048, exponent.get(), nullptr);
```
You can look at a more detailed snippet in examples/rsa.cpp. Looking at that file: If compiled with gcc 6.1.1 and optimization -O1, the CWrap way of creating an RSA keypair results in 269 bytes in the binary, whereas the "standard C way" compiles to 799 bytes.

* `Guard`  
A class used to wrap C-types so that they are always deallocated using an arbitrary function. This can be used to ensure that pointers returned from C APIs are always deallocated using the proper deallocation strategy mandates by the C API.
