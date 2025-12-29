add_library(ex_db STATIC
    db.c
    ndb.c)

target_include_directories(ex_db PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})