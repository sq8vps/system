if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
    if(DEFINED PAE AND PAE EQUAL 1)
        target_compile_definitions(config INTERFACE PAE=1)
    endif()

    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        add_custom_command(TARGET kernel POST_BUILD
            COMMAND objcopy ARGS --only-keep-debug $<TARGET_FILE:kernel> "$<TARGET_FILE:kernel>.sym"
            COMMAND objcopy ARGS --strip-debug $<TARGET_FILE:kernel>)
    endif()
endif()