@ok
<?php

function test_one_ctor_arg() {
  new class(77) {
    function __construct($num) {
      echo $num;
    }
  };
}

function test_two_ctor_arg() {
  new class("foo", "bar") {
    function __construct($s1, $s2) {
      echo $s1, $s2;
    }
  };
}

test_one_ctor_arg();
test_two_ctor_arg();
