@ok php7_4
<?php

function test(?Foo $foo) {
  nonnull_foo($foo ?: new Foo());
}

function nonnull_foo(Foo $foo) {}

class Foo {}

test(null);
test(new Foo());
