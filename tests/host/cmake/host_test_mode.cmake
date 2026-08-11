if(NOT DEFINED HOST_TEST_MODE)
  set(HOST_TEST_MODE "normal")
endif()

if(HOST_TEST_MODE STREQUAL "normal")
  return()
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  message(FATAL_ERROR
    "Host test mode '${HOST_TEST_MODE}' requires GCC or Clang; found ${CMAKE_C_COMPILER_ID}")
endif()

if(HOST_TEST_MODE STREQUAL "sanitizers")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g)
  add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
elseif(HOST_TEST_MODE STREQUAL "coverage")
  if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR
      "Coverage mode requires GCC/gcov; found ${CMAKE_C_COMPILER_ID}")
  endif()
  add_compile_options(--coverage -fprofile-abs-path -fprofile-update=atomic -O0 -g)
  add_link_options(--coverage)
else()
  message(FATAL_ERROR "Unknown HOST_TEST_MODE: ${HOST_TEST_MODE}")
endif()
