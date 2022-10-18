cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(workflow_objc)

# Add subdirectory for the Binary Ninja C++ API if performing a public build,
# and skip linking against the UI library since it isn't needed for this plugin.
if(NOT BN_INTERNAL_BUILD)
  set(HEADLESS ON CACHE BOOL "")
  add_subdirectory(Vendor/BinaryNinjaAPI)
endif()

# Core library -----------------------------------------------------------------

set(CORE_SOURCE
  Core/ExceptionUtils.h
  Core/Analyzers/CFStringAnalyzer.h
  Core/Analyzers/ClassAnalyzer.h
  Core/Analyzers/ClassRefAnalyzer.h
  Core/Analyzers/ProtocolAnalyzer.h
  Core/Analyzers/SelectorAnalyzer.h
  Core/Analyzers/SuperClassRefAnalyzer.h
  Core/BinaryViewFile.h
  Core/ABI.h
  Core/AbstractFile.h
  Core/AnalysisInfo.h
  Core/AnalysisProvider.h
  Core/Analyzer.h
  Core/TypeParser.h
  Core/Analyzers/CFStringAnalyzer.cpp
  Core/Analyzers/ClassAnalyzer.cpp
  Core/Analyzers/ClassRefAnalyzer.cpp
  Core/Analyzers/ProtocolAnalyzer.cpp
  Core/Analyzers/SelectorAnalyzer.cpp
  Core/Analyzers/SuperClassRefAnalyzer.cpp
  Core/ExceptionUtils.cpp
  Core/BinaryViewFile.cpp
  Core/ABI.cpp
  Core/AbstractFile.cpp
  Core/AnalysisInfo.cpp
  Core/AnalysisProvider.cpp
  Core/Analyzer.cpp
  Core/TypeParser.cpp)

add_library(objc_analysis_core STATIC ${CORE_SOURCE})
target_compile_features(objc_analysis_core PRIVATE cxx_std_17)

# Option to specify whether the core library should be built with support
# modules for integrating with Binary Ninja. Enabled by default, but able to be
# turned off so the core library can be built without the Binary Ninja
# dependency; an alternative AbstractFile implementation will be needed.
option(OAC_BN_SUPPORT "Build core library support for Binary Ninja" ON)
if(OAC_BN_SUPPORT)
  target_compile_definitions(objc_analysis_core PUBLIC OAC_BN_SUPPORT=1)
  target_link_libraries(objc_analysis_core PRIVATE binaryninjaapi)
endif()

# Binary Ninja plugin ----------------------------------------------------------

set(PLUGIN_SOURCE
  Commands.h
  Commands.cpp
  CustomTypes.h
  CustomTypes.cpp
  DataRenderers.h
  DataRenderers.cpp
  GlobalState.h
  GlobalState.cpp
  InfoHandler.h
  InfoHandler.cpp
  Plugin.cpp
  Workflow.h
  Workflow.cpp)

add_library(workflow_objc SHARED ${PLUGIN_SOURCE})
target_link_libraries(workflow_objc objc_analysis_core binaryninjaapi)
target_compile_features(workflow_objc PRIVATE cxx_std_17 c_std_99)

# Library targets linking against the Binary Ninja API need to be compiled with
# position-independent code on Linux.
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_compile_options(objc_analysis_core PRIVATE "-fPIC")
  target_compile_options(workflow_objc PRIVATE "-fPIC")
endif()

# Configure plugin output directory for internal builds, otherwise configure
# plugin installation for public builds.
if(BN_INTERNAL_BUILD)
  set_target_properties(workflow_objc PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${BN_CORE_PLUGIN_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BN_CORE_PLUGIN_DIR})
else()
  bn_install_plugin(workflow_objc)
endif()
