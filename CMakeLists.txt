cmake_minimum_required(VERSION 3.21)

project(newosbook-tools)

find_library(FOUNDATION Foundation)
find_library(CORE_FOUNDATION CoreFoundation)
find_library(CORE_SERVICES CoreServices)

include_directories(include/)

add_executable(csops src/csops.c)

add_executable(dtize src/dtize.c)

add_executable(dyld_example src/dyld_example.c)

add_executable(filemon src/filemon.c)

add_executable(gcd src/gcd.c)

add_executable(inject src/inject.c)

add_executable(ioreg src/ioreg.c)

add_executable(joker src/joker.c)

add_executable(jurple_console src/jurple_console.c)

add_executable(kmem src/kmem.c)

add_executable(lsdtrip src/lsdtrip.m)
target_link_libraries(lsdtrip PRIVATE ${FRAMEWORK} ${CORE_FOUNDATION} ${CORE_SERVICES})

add_executable(mistool src/mistool.c)

add_executable(utun src/utun.c)