add_library(lambdifier_test STATIC catch_main.cpp)
target_compile_options(lambdifier_test PRIVATE
  "$<$<CONFIG:Debug>:${LAMBDIFIER_CXX_FLAGS_DEBUG}>"
  "$<$<CONFIG:Release>:${LAMBDIFIER_CXX_FLAGS_RELEASE}>"
  "$<$<CONFIG:RelWithDebInfo>:${LAMBDIFIER_CXX_FLAGS_RELEASE}>"
  "$<$<CONFIG:MinSizeRel>:${LAMBDIFIER_CXX_FLAGS_RELEASE}>"
)

# Let's setup the target C++ standard, but only if the user did not provide it manually.
if(NOT CMAKE_CXX_STANDARD)
  set_property(TARGET lambdifier_test PROPERTY CXX_STANDARD 20)
endif()
set_property(TARGET lambdifier_test PROPERTY CXX_STANDARD_REQUIRED YES)
set_property(TARGET lambdifier_test PROPERTY CXX_EXTENSIONS NO)

function(ADD_LAMBDIFIER_TESTCASE arg1)
  add_executable(${arg1} ${arg1}.cpp)
  target_link_libraries(${arg1} PRIVATE lambdifier_test lambdifier)
  target_compile_options(${arg1} PRIVATE
    "$<$<CONFIG:Debug>:${LAMBDIFIER_CXX_FLAGS_DEBUG}>"
    "$<$<CONFIG:Release>:${LAMBDIFIER_CXX_FLAGS_RELEASE}>"
    "$<$<CONFIG:RelWithDebInfo>:${LAMBDIFIER_CXX_FLAGS_RELEASE}>"
    "$<$<CONFIG:MinSizeRel>:${LAMBDIFIER_CXX_FLAGS_RELEASE}>"
  )
  # Let's setup the target C++ standard, but only if the user did not provide it manually.
  if(NOT CMAKE_CXX_STANDARD)
    set_property(TARGET ${arg1} PROPERTY CXX_STANDARD 20)
  endif()
  set_property(TARGET ${arg1} PROPERTY CXX_STANDARD_REQUIRED YES)
  set_property(TARGET ${arg1} PROPERTY CXX_EXTENSIONS NO)
  add_test(${arg1} ${arg1})
endfunction()

ADD_LAMBDIFIER_TESTCASE(basic_test)
ADD_LAMBDIFIER_TESTCASE(gp_test)
ADD_LAMBDIFIER_TESTCASE(bp_test)
ADD_LAMBDIFIER_TESTCASE(taylor_test)
