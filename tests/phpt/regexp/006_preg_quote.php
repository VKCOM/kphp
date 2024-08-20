@ok k2_skip
<?php

// Starting with PHP 7.3, preg_quote does escape '#' sign.

function test1() {
  $chars = range(chr(0), chr (255));
  foreach ($chars as $ch) {
    var_dump(["$ch(" . ord($ch) . ')' => preg_quote($ch)]);
  }
}

function test2() {
  $range = implode ('', range (chr(0), chr (255)));
  var_dump (preg_quote ($range));
  var_dump (preg_quote ($range, '/'));
}

test1();
test2();
