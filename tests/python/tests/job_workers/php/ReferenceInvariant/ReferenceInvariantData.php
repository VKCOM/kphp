<?php

namespace ReferenceInvariant;

abstract class ReferenceInvariantData {
  /** @var TestClassAB|null */
  public $ab = null;

  /** @var TestInterfaceA[] */
  public $a = [];

  /** @var TestInterfaceB[] */
  public $b = [];

  function init_data() {
    $this->ab = new TestClassAB();
    $this->a = [
      $this->ab,
      new TestClassA(10),
      $this->ab,
      $this->ab->a[0],
      $this->ab->a[2],
      $this->ab->ab['a'][1],
    ];
    $this->b = [
      $this->ab,
      $this->ab->b_arr[0],
      $this->ab->b[0][1],
      $this->ab->b[0][2],
      new TestClassB(11),
      $this->ab->ab['b']['arr'][1],
    ];
  }

  function verify_data() {
    $assert = function (bool $ok, string $msg) {
      if (!$ok) {
        critical_error("$msg failed");
      }
    };

    $assert($this->ab === $this->a[0], "case ab 1");
    $assert($this->ab === $this->b[0], "case ab 2");
    $assert($this->ab->b_arr["key 74"] === $this->ab->b_arr["key 10"], "case ab 3");
    $assert($this->ab->b_arr["key 100"] === $this->ab->ab['b']['arr'][2], "case ab 4");

    $assert($this->a[0] === $this->a[2], "case a 1");
    $assert($this->a[3] === $this->ab->a[0], "case a 2");
    $assert($this->a[5] === $this->ab->ab['a'][1], "case a 3");

    $assert($this->b[1] === $this->ab->b_arr[0], "case b 1");
    $assert($this->b[3] === $this->ab->b[0][2], "case b 2");
    $assert($this->b[5] === $this->ab->ab['b']['arr'][1], "case b 3");

    /** @var TestClassAB $a */
    $a = instance_cast($this->a[0], TestClassAB::class);
    $assert($a !== null, "common case 1");
    $assert($a->huge_arr["key 17"] === "value 17", "common case 2");

    /** @var TestClassAB $b */
    $b = instance_cast($this->b[0], TestClassAB::class);
    $assert($b !== null, "common case 3");
    $assert($b->huge_arr["key 94"] === "value 94", "common case 4");
  }
}
