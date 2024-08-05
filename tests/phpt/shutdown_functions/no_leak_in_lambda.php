@ok k2_skip
<?php

function test_no_leak_in_shutdown_functions() {
  $a = 'bar';
  $b = 'bam';
  $c = 'baf';
  register_shutdown_function(function() use($a, $b) { echo $a, $b, "\n"; });
  register_shutdown_function(function() use($a, $b, $c) { echo $a, $b, $c, "\n"; });
}

test_no_leak_in_shutdown_functions();
