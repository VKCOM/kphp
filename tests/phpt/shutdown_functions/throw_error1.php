@kphp_should_fail
/register_shutdown_callback should not throw exceptions/
<?php

/** @kphp-required */
function throwing_func() {
  throw new Exception('BAD');
}

register_shutdown_function('throwing_func');
