@ok
<?php

class A {
  public $a = 1;
}

class A1 extends A {
  public $a1 = 1;
}

class A2 extends A {
  public $a2 = 1;
}

/** @param A1|A2|null $x */
function test1($x) {
  if ($x instanceof A1) {
    var_dump($x->a1);
    var_dump($x->a);
  }
  if ($x instanceof A2) {
    var_dump($x->a2);
    var_dump($x->a);
  }
}

/** @param A1|null|A2 $x */
function test2($x) {
  if ($x instanceof A1) {
    var_dump($x->a1);
    var_dump($x->a);
  }
  if ($x instanceof A2) {
    var_dump($x->a2);
    var_dump($x->a);
  }
}

/** @param A1|null $x */
function test3($x) {
  if ($x instanceof A1) {
    var_dump($x->a1);
    var_dump($x->a);
  }
}

/** @param A|null $x */
function test4($x) {
  if ($x instanceof A1) {
    var_dump($x->a1);
    var_dump($x->a);
  }
  if ($x instanceof A2) {
    var_dump($x->a2);
    var_dump($x->a);
  }
}

test1(new A1());
test1(new A2());
test1(null);

test2(new A1());
test2(new A2());
test2(null);

test3(new A1());
test3(null);

test4(new A1());
test4(new A2());
test4(null);

class StrangeThis {
    function isActive() { return true; }

    function fun() {
        // test that smart casts to $this work correctly
        if (!$this || !$this->isActive()) return false;
        return true;
    }
}

var_dump((new StrangeThis)->fun());

