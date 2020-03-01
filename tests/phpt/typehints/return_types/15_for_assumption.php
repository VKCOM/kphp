@ok
<?php

class A {
  /** @var int */
  public $a = 0;

  function __construct($a = 0) { $this->a = $a; }
}

function f1(A $a) : A {
  echo $a->a, "\n";
  if(true) { return $a; }
  return null;
}

$a1 = f1(new A);
echo $a1->a + 1, "\n";

function f2() : ?A {
  if(true) return new A(7);
  else return null;
}

echo f2()->a + 1, "\n";



