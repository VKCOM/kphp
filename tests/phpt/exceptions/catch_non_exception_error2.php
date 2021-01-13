@kphp_should_fail
/Expected type that implements Throwable, found 'int'/
<?php

try {
  var_dump(0);
} catch (int $e) {
}
