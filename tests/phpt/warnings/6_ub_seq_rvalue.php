@kphp_should_warn
/Dangerous undefined behaviour string build, \[var = \$s1\]/
/Dangerous undefined behaviour string build, \[var = \$s2\]/
<?php

// We generate a seq_rvalue for the specialized preg_match calls.
// UB should be detected inside the seq_rvalue block.

function test1() {
  preg_match((($s1 = '') . $s1 . '/\d/'), '', $m);
}

function test2() {
  preg_match('/\d/', (($s2 = '') . $s2 . '13'), $m);
}

test1();
test2();
