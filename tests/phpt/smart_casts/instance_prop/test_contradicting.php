@ok php7_4
<?php

function test(Foo $foo) {
  if (!isset($foo->bar)) {
    $foo = new Foo();
    if (isset($foo->bar)) {
      nonnull_bar($foo->bar);
    }
  }
}

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;
}

class Bar {}

test(new Foo());
