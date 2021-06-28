@ok
<?php

function test_set_to_variable() {
  $a = new class {
    function say_foo() {
      echo "foo";
    }
  };
  $a->say_foo();
}

test_set_to_variable();
