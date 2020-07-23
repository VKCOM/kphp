@kphp_should_fail
/Result of function call is unused, but function is marked with @kphp-warn-unused-result/
<?php
/**
 * @kphp-warn-unused-result
 */
function test(): int {
  return 777;
}

test();

