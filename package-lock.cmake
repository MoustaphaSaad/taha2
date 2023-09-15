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
  GIT_TAG 10.1.1
  GIT_REPOSITORY git@github.com:fmtlib/fmt.git
  GIT_SHALLOW TRUE
  EXCLUDE_FROM_ALL TRUE
)