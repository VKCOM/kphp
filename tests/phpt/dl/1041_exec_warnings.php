@kphp_runtime_should_warn
/Cannot execute a blank command/
/Cannot execute a blank command/
<?php

function test_exec_warnings() {
  system('');
  exec('');
  passthru('');
}

test_exec_warnings();
