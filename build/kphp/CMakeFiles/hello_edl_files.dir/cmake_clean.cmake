file(REMOVE_RECURSE
  "../_headers_/kphp/Kphp.edl"
  "../_headers_/kphp/Kphp.edl.h"
  "Kphp.edl.h"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/hello_edl_files.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
