@kphp_should_fail k2_skip
/header_register_callback should not throw exceptions/
<?php

/** @kphp-required */
function throwing_func() {
  throw new Exception('BAD');
}

header_register_callback('throwing_func');
