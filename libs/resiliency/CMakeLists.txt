# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $HPX_ROOT/libs/resiliency/include
set(resiliency_headers
  hpx/resiliency/async_replay.hpp
  hpx/resiliency/async_replicate.hpp
  hpx/resiliency/config.hpp
  hpx/resiliency/dataflow_replay.hpp
  hpx/resiliency/dataflow_replicate.hpp
  hpx/resiliency/force_linking.hpp
  hpx/resiliency/version.hpp
  hpx/resiliency/resiliency.hpp
)

# Default location is $HPX_ROOT/libs/resiliency/src
set(resiliency_sources
  force_linking.cpp
  resiliency.cpp
)

include(HPX_AddModule)
add_hpx_module(resiliency
  GLOBAL_HEADER_GEN OFF
  SOURCES ${resiliency_sources}
  HEADERS ${resiliency_headers}
  DEPENDENCIES
    hpx_assertion
    hpx_config
    hpx_datastructures
    hpx_local_async
  CMAKE_SUBDIRS examples tests
)
