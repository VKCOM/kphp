@ok
<?php

class A {
  public $a = 0;

  /**
   * @kphp-infer
   * @param int $a
   */
  public function __construct($a = 0) {
    $this->a = $a;
  }
}

class AContainer {
  /** @var A[][]|null */
  public $a2 = null;
  /** @var A[][][]|false */
  public $a3 = false;
}

/**
 * @return A[]
 */
function getA1() {
  return [
    new A(1), new A(2),
  ];
}

function demo1() {
  $a1 = getA1();
  foreach($a1 as $a) {
    echo $a->a, " ";
  }
  echo "\n\n";
}

/**
 * @return A[][]
 */
function getA2() {
  return [
    [new A(11), new A(12),],
    [new A(21), new A(22),],
  ];
}

function demo2() {
  $a2 = getA2();
  foreach($a2 as $a1) {
    foreach($a1 as $a) {
      echo $a->a, ' ';
    }
    echo "\n";
  }
  echo "\n\n";
}

/**
 * @return A[][][]
 */
function getA3() {
  return [
    [
      [new A(111), new A(112),],
      [new A(121), new A(122),],
    ],
    [
      [new A(211), new A(212),],
      [new A(221), new A(222),],
    ],
   ];
}

function demo3() {
  $a3 = getA3();
  foreach($a3 as $a2) {
    foreach($a2 as $a1) {
      foreach($a1 as $a) {
        echo $a->a, ' ';
      }
      echo "\n";
    }
    echo "\n";
  }
  echo "\n\n";
}

function demo3_v2() {
  $c = new AContainer;
  $c->a3 = getA3();
  foreach($c->a3 as $a2) {
    foreach($a2 as $a1) {
      foreach($a1 as $a) {
        echo $a->a, ' ';
      }
      echo "\n";
    }
    echo "\n";
  }
  echo "\n\n";
  $c->a2 = $c->a3[0];
}

/** @param null|A[][][] $a3 */
function demo3_v3($a3) {
  foreach($a3 as $a2) {
    foreach($a2 as $a1) {
      foreach($a1 as $a) {
        echo $a->a, ' ';
      }
      echo "\n";
    }
    echo "\n";
  }
  echo "\n\n";
}



demo1();
demo2();
demo3();
demo3_v2();
demo3_v3(getA3());
