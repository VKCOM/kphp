// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// enum for type_out()
enum class gen_out_style {
  cpp,    // for C++ code
  txt,    // for functions.txt in lib export and some type comparison todo get rid of it
  tagger  // for C++ code inside storage tagger (types available from forks, futures are ints there)

  // to output a type to console, use type->as_human_readable(), not type_out()
};
