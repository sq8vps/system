add_subdirectory(db)
add_subdirectory(kdrv)

add_library(ex OBJECT
    elf.c
    exec.c
    ksym.c
    load.c
    worker.c
    )

target_include_directories(ex PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(ex PUBLIC
    ex_db $<TARGET_OBJECTS:ex_db>
    ex_kdrv $<TARGET_OBJECTS:ex_kdrv>
)
target_link_libraries(ex PRIVATE kernel_config)