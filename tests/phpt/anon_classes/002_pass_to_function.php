@ok
<?php

interface I {
  function say_foo();
}

function call_say_foo(I $i) {
  $i->say_foo();
}

function test_pass_to_function() {
  call_say_foo(new class implements I {
    function say_foo() {
      echo "foo";
    }
  });
}

test_pass_to_function();
