@ok php8
<?php

try {
  throw new Exception();
} catch (Exception) {
  echo "Exception\n";
}

try {
  throw new Exception();
} catch (Exception) {
  echo "Exception\n";
} catch (Error) {
  echo "FAIL\n";
}
