@kphp_should_fail
/passed nullable Foo value as \$foo argument to nonnull_foo/
<?php

function nonnull_foo(Foo $foo) {}

function new_nullable_foo(): ?Foo { return null; }

class Foo {
  public ?Foo $next;
}

function test(Foo $foo) {
  if (is_null($foo->next)) {
    $foo->next = new Foo();
  }
  $foo->next = new_nullable_foo();
  nonnull_foo($foo->next);
}

test(new Foo());
