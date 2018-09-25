#------------------------------------------------------------------------------
# Setting compiling for FISCO-BCOS.
# ------------------------------------------------------------------------------
# This file is part of FISCO-BCOS.
#
# FISCO-BCOS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# FISCO-BCOS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2016-2018 fisco-dev contributors.
#------------------------------------------------------------------------------

# common settings
set(ETH_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(ETH_SCRIPTS_DIR ${ETH_CMAKE_DIR}/scripts)

if (("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang"))
    # Use ISO C++11 standard language.
    set(CMAKE_CXX_FLAGS -std=c++11)

    # Enables all the warnings about constructions that some users consider questionable,
    # and that are easy to avoid.  Also enable some extra warning flags that are not
    # enabled by -Wall.   Finally, treat at warnings-as-errors, which forces developers
    # to fix warnings as they arise, so they don't accumulate "to be fixed later".
    add_compile_options(-Wall)
    add_compile_options(-Wno-unused-variable)
    add_compile_options(-Wno-missing-field-initializers)
    add_compile_options(-Wno-unused-parameter)
    add_compile_options(-Wextra)
    # for boost json spirit
    add_compile_options(-DBOOST_SPIRIT_THREADSAFE)
    # for easy log
    add_compile_options(-DELPP_THREAD_SAFE)

    add_compile_options(-Wa,-march=generic64)

    if(STATIC_BUILD)
        SET(BUILD_SHARED_LIBRARIES OFF)
        SET(CMAKE_EXE_LINKER_FLAGS "-static")
    endif ()

    # Disable warnings about unknown pragmas (which is enabled by -Wall).
    add_compile_options(-Wno-unknown-pragmas)
    
    add_compile_options(-fno-omit-frame-pointer)

	# Configuration-specific compiler settings.
    set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g -DETH_DEBUG -DELPP_THREAD_SAFE")
    set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG -DETH_RELEASE -DELPP_THREAD_SAFE")
    set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -DNDEBUG -DETH_RELEASE -DELPP_THREAD_SAFE")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DETH_RELEASE -DELPP_THREAD_SAFE")

    option(USE_LD_GOLD "Use GNU gold linker" ON)
    if (USE_LD_GOLD)
        execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
        if ("${LD_VERSION}" MATCHES "GNU gold")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")
        endif ()
    endif ()

    # Additional GCC-specific compiler settings.
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")

        # Check that we've got GCC 4.7 or newer.
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
        if (NOT (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7))
            message(FATAL_ERROR "${PROJECT_NAME} requires g++ 4.7 or greater.")
        endif ()

		# Strong stack protection was only added in GCC 4.9.
		# Use it if we have the option to do so.
		# See https://lwn.net/Articles/584225/
        if (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
            add_compile_options(-fstack-protector-strong)
            add_compile_options(-fstack-protector)
        endif()
    # Additional Clang-specific compiler settings.
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        add_compile_options(-fstack-protector)
        # Some Linux-specific Clang settings.  We don't want these for OS X.
        if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
            # Tell Boost that we're using Clang's libc++.   Not sure exactly why we need to do.
            add_definitions(-DBOOST_ASIO_HAS_CLANG_LIBCXX)
            # Use fancy colors in the compiler diagnostics
            add_compile_options(-fcolor-diagnostics)
        endif()
    endif()
# If you don't have GCC, Clang then you are on your own.  Good luck!
else ()
    message(WARNING "Your compiler is not tested, if you run into any issues, we'd welcome any patches.")
endif ()

if (SANITIZE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=${SANITIZE}")
    if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/sanitizer-blacklist.txt")
    endif()
endif()


if (COVERAGE)
    set(CMAKE_CXX_FLAGS "-g --coverage ${CMAKE_CXX_FLAGS}")
    set(CMAKE_C_FLAGS "-g --coverage ${CMAKE_C_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "--coverage ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "--coverage ${CMAKE_EXE_LINKER_FLAGS}")
    find_program(LCOV_TOOL lcov)
    message(STATUS "lcov tool: ${LCOV_TOOL}")
    if (LCOV_TOOL)
        add_custom_target(coverage.info
            COMMAND ${LCOV_TOOL} -o ${CMAKE_BINARY_DIR}/coverage.info -c -d ${CMAKE_BINARY_DIR}
            COMMAND ${LCOV_TOOL} -o ${CMAKE_BINARY_DIR}/coverage.info -r ${CMAKE_BINARY_DIR}/coverage.info '/usr*' '${CMAKE_BINARY_DIR}/deps/*' '${CMAKE_SOURCE_DIR}/deps/*'
    )
    endif()
endif ()
