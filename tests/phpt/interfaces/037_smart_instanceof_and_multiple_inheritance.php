@ok
<?php

interface A { public function fn_a(); }
interface B { public function fn_b(); }

class C implements A, B {
  public function fn_a() { echo 'a'; }
  public function fn_b() { echo 'b'; }
  public function fn_c() { echo 'c'; }
}

function bar(B $b) {
    $b->fn_b();
}

function foo(A $a) {
  if ($a instanceof B) {
    $a->fn_a();
    bar($a);
    // It must fail a compilation process, but it doesn't. Hope to fix it in the future
    $a->fn_c();
  }
}

foo(new C());
