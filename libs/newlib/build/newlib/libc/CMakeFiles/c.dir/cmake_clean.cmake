file(REMOVE_RECURSE
  "libc.pdb"
  "libc.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/c.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
