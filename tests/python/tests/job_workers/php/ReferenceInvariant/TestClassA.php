<?php

namespace ReferenceInvariant;

class TestClassA implements TestInterfaceA {
  function __construct(int $size) {
    while (--$size > 0) {
      $this->a["A key $size"] = "A value $size";
    }
  }

  /** @var string[] */
  public $a = [];
}
