#
#   CWrap - A library to use C-code in C++.
#   Copyright (C) 2016  Marcus Gelderie
#
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
#   USA


find_package(OpenSSL)

set(COMPILE_OPTIONS "-O2")

if(OPENSSL_FOUND)
    add_executable(rsa_keypair rsa.cpp)
    target_link_libraries(rsa_keypair ${OPENSSL_LIBRARIES} CWrap)
    target_compile_options(rsa_keypair PRIVATE ${COMPILE_OPTIONS})
else(OPENSSL_FOUND)
    message(STATUS "OpenSSL not available. Not building examples with OpenSSL dependecy.")
endif(OPENSSL_FOUND)
