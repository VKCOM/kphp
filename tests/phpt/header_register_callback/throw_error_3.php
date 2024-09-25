@kphp_should_fail k2_skip
/header_register_callback should not throw exceptions/
<?php

function may_throw(bool $cond) {
  if ($cond) {
    throw new Exception('BAD');
  }
}

header_register_callback(function () {
  may_throw(false);
});
