file(REMOVE_RECURSE
  "../../kphp/objs/libkphp-full-runtime.a"
  "../../kphp/objs/libkphp-full-runtime.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/kphp-full-runtime.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
