@kphp_should_fail
/header_register_callback should not throw exceptions/
<?php

header_register_callback(function () {
  throw new Exception('BAD');
});
