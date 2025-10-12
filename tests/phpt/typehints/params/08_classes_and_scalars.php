@ok k2_skip
<?php
class A {
  public function empty() {}
}

function f1(array $arr, A $a) {
  $a->empty();
}
f1([], new A);

function f2(?array $arr, A $a) {
  $a->empty();
}
f2(null, new A);

function f3(string $arr, A $a) {
  $a->empty();
}
f3("", new A);

function f4(array $arr, A $a, int ...$ints) {
  $a->empty();
}
f4([], new A());
