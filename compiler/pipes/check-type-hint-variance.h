// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"

// CheckTypeHintVariance checks that type hints that were used in
// child methods are compatible with PHP and would not lead to a fatal error.
//
// In KPHP, type information can be provided from both phpdocs and type hints;
// in PHP we can only use type hints. KPHP checks the signature types compatibility
// using the full type info (phpdoc + type hints) while PHP relies on type hints only.
// This means that sometimes KPHP can allow omitting the type hint as the information
// can be inferred from the phpdoc. That would be incompatible with PHP as you
// would get a fatal error for such code. This pass checks that we have a high chance
// of being compatible with PHP in terms of type hints placement, but it doesn't
// perform an actual signature types comparison (it happens later in the pipeline).
//
// https://www.php.net/manual/en/language.oop5.variance.php
class CheckTypeHintVariance {
public:
  void execute(FunctionPtr interface_function, DataStream<FunctionPtr>& os);
};
