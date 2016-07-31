find_package(OpenSSL)

if(DEFINED OpenSSL_FOUND)
    add_executable(rsa_keypair rsa.cpp)
    target_link_libraries(rsa_keypair ${OPENSSL_LIBRARIES} CWrap)
else(DEFINED OpenSSL_FOUND)
    message(STATUS "OpenSSL not available. Not building examples with OpenSSL dependecy.")
endif(DEFINED OpenSSL_FOUND)
