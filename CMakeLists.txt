cmake_minimum_required(VERSION 3.21)

project(newosbook-tools)

find_library(FOUNDATION Foundation)
find_library(CORE_FOUNDATION CoreFoundation)
find_library(CORE_SERVICES CoreServices)
find_library(MOBILE_DEVICE MobileDevice)
find_library(LIBMIS mis)

include_directories(include/)

add_executable(csops src/csops.c)

add_executable(dtize src/dtize.c)

add_executable(dyld_example src/dyld_example.c)

# add_executable(filemon src/filemon.c)
# target_link_libraries(filemon PRIVATE ${CORE_SERVICES})

add_executable(gcd src/gcd.c)

# add_executable(inject src/inject.c)

# add_executable(ioreg src/ioreg.c)

# add_executable(joker src/joker.c)

# add_executable(jurple_console src/jurple_console.c)
# target_link_libraries(jurple_console PRIVATE ${MOBILE_DEVICE})

add_executable(kmem src/kmem.c)

add_executable(lsdtrip src/lsdtrip.m)
target_link_libraries(lsdtrip PRIVATE ${FRAMEWORK} ${CORE_FOUNDATION} ${CORE_SERVICES})

# add_executable(lsock src/lsock.c)

add_executable(lstacks src/lstacks.c)

add_executable(mach_task_for_pid src/mach_task_for_pid.c)

add_executable(mach_task_for_self src/mach_task_for_self.c)

add_executable(mistool src/mistool.c)
target_link_libraries(mistool ${CORE_FOUNDATION} ${LIBMIS})

add_executable(utun src/utun.c)