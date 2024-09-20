add_executable(librestation-regtest
  regtest_host.cpp
)

target_link_libraries(librestation-regtest PRIVATE core common scmversion)

add_core_resources(librestation-regtest)
