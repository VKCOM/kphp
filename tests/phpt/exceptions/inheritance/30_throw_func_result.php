@ok
<?php

function get_exception() {
  return new Exception();
}

try {
    throw get_exception();
} catch (Exception $ex) {
    echo "caught\n";
}

