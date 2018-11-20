@kphp_should_fail
<?php

function test() {
    $x = 10;
    $u = function ($x) use ($x) {
      return $x;
    };
    var_dump($u(100));
}

test();
