@kphp_should_fail
/finally is unsupported/
<?php

try {
  echo "try\n";
} finally {
  echo "finally\n";
}
