add_library(ex_db OBJECT
    db.c
    ndb.c)

target_include_directories(ex_db PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(ex_db PRIVATE kernel_config)