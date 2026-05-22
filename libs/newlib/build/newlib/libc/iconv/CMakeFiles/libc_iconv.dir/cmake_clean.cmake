file(REMOVE_RECURSE
  "liblibc_iconv.a"
  "liblibc_iconv.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/libc_iconv.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
