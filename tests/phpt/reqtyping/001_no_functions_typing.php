@ok
<?php

// by default, KPHP_REQUIRE_FUNCTIONS_TYPING = 0, in tests also

// arguments typing not mandatory

function f1($a, $b) {}

f1(0, 0);
f1('0', '0');

// though they can be partially set and will be checked

function f2(int $a, $b) {}

f2(0, 0);
f2(0, '0');

// if no @return — auto infer, not void

function f3() { return 5; }

f3();

/**
 * @param int|false $a
 * @return int
 */
function f4($a, $b, ?string $c) {
  return (int)$a;
}

f4(false, [1,2,3], null);
