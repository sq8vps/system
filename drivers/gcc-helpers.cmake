function(export_symbols)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        add_custom_command(TARGET ${DRIVER} POST_BUILD
            COMMAND objcopy ARGS --only-keep-debug $<TARGET_FILE:${DRIVER}> "$<TARGET_FILE:${DRIVER}>.sym"
            COMMAND objcopy ARGS --strip-debug $<TARGET_FILE:${DRIVER}>)
    endif()
endfunction()

function(export_driver)
    add_custom_command(TARGET ${DRIVER} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${DRIVER}> ${OS_IMAGE_DIR}/drivers/bin/$<TARGET_FILE_NAME:${DRIVER}>
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${DRIVER}.ndb ${OS_IMAGE_DIR}/drivers/db/${DRIVER}.ndb
    )
endfunction()

function(export_initrd_driver)
    add_custom_command(TARGET ${DRIVER} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${DRIVER}> ${INITRD_DIR}/drivers/bin/$<TARGET_FILE_NAME:${DRIVER}>
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${DRIVER}.ndb ${INITRD_DIR}/drivers/db/${DRIVER}.ndb
    )
endfunction()