add_library(libc_iconv_ccs OBJECT
    ccsbi.c
    cns11643_plane2.c
    cp866.c
    iso_8859_14.c
    iso_8859_5.c
    iso_ir_111.c
    koi8_ru.c
    win_1250.c
    win_1255.c
    cp775.c
    iso_8859_1.c
    iso_8859_15.c
    iso_8859_6.c
    jis_x0201_1976.c
    koi8_u.c
    win_1251.c
    win_1256.c
    big5.c
    cp850.c
    iso_8859_10.c
    iso_8859_2.c
    iso_8859_7.c
    jis_x0208_1990.c
    koi8_uni.c
    win_1252.c
    win_1257.c
    cns11643_plane1.c
    cp852.c
    iso_8859_11.c
    iso_8859_3.c
    iso_8859_8.c
    jis_x0212_1990.c
    ksx1001.c
    win_1253.c
    win_1258.c
    cns11643_plane14.c
    cp855.c
    iso_8859_13.c
    iso_8859_4.c
    iso_8859_9.c
    koi8_r.c
    win_1254.c
)

target_include_directories(libc_iconv_ccs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_iconv_ccs PRIVATE libc_includes)