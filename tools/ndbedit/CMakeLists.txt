add_executable(ndbedit 
    main.c
    ndb.c
)
target_include_directories(ndbedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})