@ok
<?php

#ifndef KPHP
  function critical_error($msg){}
#endif

function test_no_leak_in_shutdown_functions_with_error() {
  $a = 'bar';
  $b = 'bam';
  $c = 'baf';
  register_shutdown_function(function() use($a, $b) { echo $a, $b, "\n"; });
  register_shutdown_function(function() use($a, $b, $c) {
    echo $a, $b, $c, "\n";
    critical_error("critical error from shutdown function");
  });
}

test_no_leak_in_shutdown_functions_with_error();
