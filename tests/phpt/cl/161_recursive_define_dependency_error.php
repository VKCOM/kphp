@kphp_should_fail
\Recursive define dependency\
<?php

class Z {
   const Y = X::Y;
}

class X {
   const Y = Z::Y;
}

function test() {
  var_dump(X::Y);
}

test();
