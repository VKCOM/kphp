@kphp_should_fail 
/register_shutdown_function should not throw exceptions/
<?php

register_shutdown_function(function () {
  throw new Exception('BAD');
});
