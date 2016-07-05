[![Build Status](https://travis-ci.org/mgelde/CWrap.svg?branch=master)](https://travis-ci.org/mgelde/CWrap)

CWrap - Wrap C-code easily.
=====

Introduction
--------

CWrap is a header-only library designed to more easily use C-code from C++. It provides helper classes and methods to bring type and memory safety to C APIs. It also helps to avoid boilerplate code that typically stems from error handling in C.

Currently, CWrap provides the following features:  
* `Guard`  
A class used to wrap C-types so that they are always deallocated using an arbitrary function. This can be used to ensure that pointers returned from C APIs are always deallocated using the proper deallocation strategy mandates by the C API.
* `CallGuard`  
A class to wrap C-function-calls and transparently manage error handling. This can be used to transparently, and
consistently, verify the success of function calls, and to take appropriate action if the verification fais.
