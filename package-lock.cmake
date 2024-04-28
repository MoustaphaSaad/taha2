# CPM Package Lock
# This file should be committed to version control

# doctest
CPMDeclarePackage(doctest
  NAME doctest
  GIT_TAG v2.4.11
  GIT_REPOSITORY git@github.com:doctest/doctest.git
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL TRUE
)

CPMDeclarePackage(utf8proc
  NAME utf8proc
  GIT_TAG v2.8.0
  GIT_REPOSITORY git@github.com:JuliaStrings/utf8proc.git
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL TRUE
)

CPMDeclarePackage(fmt
  NAME fmt
  GIT_TAG 10.2.1
  GIT_REPOSITORY git@github.com:fmtlib/fmt.git
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL TRUE
  OPTIONS
    "FMT_USE_LOCAL_TIME ON"
)

CPMDeclarePackage(spdlog
  NAME spdlog
  GIT_TAG v1.13.0
  GIT_REPOSITORY git@github.com:gabime/spdlog.git
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL TRUE
  OPTIONS
    "SPDLOG_FMT_EXTERNAL ON"
    "SPDLOG_NO_EXCEPTIONS ON"
)

CPMDeclarePackage(libressl
  NAME libressl
  URL https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.8.2.tar.gz
  EXCLUDE_FROM_ALL TRUE
  SYSTEM
  OPTIONS
    "LIBRESSL_SKIP_INSTALL ON"
    "LIBRESSL_APPS OFF"
    "LIBRESSL_TESTS OFF"
    "ENABLE_ASM ${ENABLE_ASM}"
)

CPMDeclarePackage(tracy
  NAME tracy
  GIT_TAG v0.10
  GIT_REPOSITORY git@github.com:wolfpld/tracy.git
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL TRUE
  OPTIONS
    "TRACY_ENABLE ${TRACY_ENABLE}"
    "TRACY_CALLSTACK ${TRACY_ENABLE}"
    "TRACY_NO_SAMPLING ${TRACY_ENABLE}"
    "TRACY_ON_DEMAND ${TRACY_ENABLE}"
)

set(TAHA2_CPPTRACE_USE_WINAPI_UNWIND OFF)
if (MINGW OR WIN32)
  set(TAHA2_CPPTRACE_USE_WINAPI_UNWIND ON)
endif ()

CPMDeclarePackage(cpptrace-lib
  NAME cpptrace-lib
  GIT_TAG v0.5.0
  GIT_REPOSITORY git@github.com:jeremy-rifkin/cpptrace.git
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL TRUE
  OPTIONS
    "CPPTRACE_UNWIND_WITH_WINAPI ${TAHA2_CPPTRACE_USE_WINAPI_UNWIND}"
)

CPMDeclarePackage(date
  NAME date
  GIT_TAG v3.0.1
  GIT_REPOSITORY git@github.com:HowardHinnant/date.git
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL TRUE
)

CPMDeclarePackage(glfw
  NAME glfw
  GIT_TAG 3.4
  GIT_REPOSITORY git@github.com:glfw/glfw.git
  GIT_SHALLOW TRUE
  # EXCLUDE_FROM_ALL TRUE
  OPTIONS
    "GLFW_BUILD_EXAMPLES OFF"
    "GLFW_BUILD_TESTS OFF"
    "GLFW_BUILD_DOCS OFF"
    "GLFW_INSTALL OFF"
)

CPMDeclarePackage(nanobench
  NAME nanobench
  GIT_TAG v4.3.11
  GIT_REPOSITORY git@github.com:martinus/nanobench.git
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL TRUE
)
