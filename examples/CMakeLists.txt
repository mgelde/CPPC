find_package(OpenSSL)

if(OpenSSL_FOUND)
    add_executable(rsa_keypair rsa.cpp)
    target_link_libraries(rsa_keypair ${OPENSSL_LIBRARIES} CWrap)
else(OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not available. Not building examples with OpenSSL dependecy.")
endif(OpenSSL_FOUND)
