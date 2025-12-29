add_subdirectory(db)
add_subdirectory(kdrv)

add_library(ex STATIC
    elf.c
    exec.c
    ksym.c
    load.c
    worker.c
    )

target_include_directories(ex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ex PUBLIC
    ex_db
    ex_kdrv
    )