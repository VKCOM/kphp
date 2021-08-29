<?php

namespace ReferenceInvariant;

abstract class TestAbstractClassA implements TestInterfaceA {
  function __construct(TestInterfaceA $a1, TestInterfaceA $a2, TestInterfaceB $b1, TestInterfaceB $b2) {
    $this->a = [$a1, $a2, $a1, $a1, $a2, $this, $this];
    for ($i = 0; $i != 100; ++$i) {
      array_push($this->a, $a1);
      array_push($this->a, $a2);
      array_push($this->a, $this);
    }
    $this->b = tuple([$b1, $b2, $b1, $b1, $b2], 100);
    $this->ab = shape([
      'a' => tuple(10, $a2),
      'b' => shape([
        'arr' => [$b1, $b1, $b2, $b2, $b2, $b2]
      ])
    ]);
    for ($i = 0; $i != 100; ++$i) {
      $this->huge_arr["key $i"] = "value $i";
    }
  }

  /** @var TestInterfaceA[] */
  public $a = [];

  /** @var tuple(TestInterfaceB[]|null, int) */
  public $b;

  /** @var shape(a:tuple(int, TestInterfaceA), b:shape(arr:TestInterfaceB[]|false)) */
  public $ab;

  /** @var string[] */
  public $huge_arr = [];
}
