@kphp_should_fail
/register_shutdown_callback should not throw exceptions/
<?php

function may_throw(bool $cond) {
  if ($cond) {
    throw new Exception('BAD');
  }
}

register_shutdown_function(function () {
  may_throw(false);
});
