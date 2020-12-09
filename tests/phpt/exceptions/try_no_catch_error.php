@kphp_should_fail
/Expected at least 1 'catch' statement/
<?php

try {
  throw new Exception('123');
}
