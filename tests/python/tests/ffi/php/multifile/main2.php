<?php

function main2() {
  $foolib = FooLib::instance();
  $foo = $foolib->new_foo();

  $foo->b = true;
  FooUtils::dump($foo);
  $foo->b = false;
  FooUtils::dump($foo);
}

main2();
