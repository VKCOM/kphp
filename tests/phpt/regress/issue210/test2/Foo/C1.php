<?php

namespace Foo;

// C3 will not be loaded ("required") from the phpdoc, so we need to add this require_once here.
require_once 'C3.php';

class C1 {
  /** @param C3 $x */
  public function f($x) {}
}

