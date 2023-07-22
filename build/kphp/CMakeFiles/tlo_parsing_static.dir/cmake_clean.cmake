file(REMOVE_RECURSE
  "libtlo_parsing.a"
  "libtlo_parsing.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/tlo_parsing_static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
