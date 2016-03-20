CWrap - Wrap C-code easily.
=====

Introduction
--------

CWrap is a header-only library designed to more easily use C-code from C++. It provides helper classes and methods to bring type and memory safety to C APIs.

Currently, CWrap provides the followin features:  
* `Guard`  
A class used to wrap C-types so that they are always deallocated using an arbitrary function. This can be used to ensure that pointers returned from C APIs are always deallocated using the proper deallocation strategy mandates by the C API.
