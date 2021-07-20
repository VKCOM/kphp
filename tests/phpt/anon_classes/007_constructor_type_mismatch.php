@kphp_should_fail
/pass int to argument \$num of anon_class.*::__construct\nbut it's declared as @param string/
<?php

function test_constructor_type_mismatch() {
  new class(42) {
    function __construct(string $num) {}
  };
}

test_constructor_type_mismatch();
