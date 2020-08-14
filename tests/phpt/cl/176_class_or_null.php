<?php
class Foo {

  public function f(): void {
    echo 'foo';
  }
}

function foo(?Foo $foo = null): void {
  if ($foo) {
    $foo->f();
  }
}

foo(new Foo());
