<?php

namespace ReferenceInvariant;

class TestClassB implements TestInterfaceB {
  function __construct(int $size) {
    while (--$size > 0) {
      $this->b["B key $size"] = "B value $size";
    }
  }

  /** @var string[] */
  public $b = [];
}
