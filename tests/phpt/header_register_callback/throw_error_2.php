@kphp_should_fail k2_skip
/header_register_callback should not throw exceptions/
<?php

header_register_callback(function () {
  throw new Exception('BAD');
});