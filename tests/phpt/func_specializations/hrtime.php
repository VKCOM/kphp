@ok
<?php

function ensure_int(int $x) {}

/** @param int[] $x */
function ensure_array($x) {}

const HRTIME_AS_INT = true;
const HRTIME_AS_ARRAY = false;

function test() {
  ensure_int(hrtime(true));
  ensure_int(hrtime(HRTIME_AS_INT));
  ensure_array(hrtime(false));
  ensure_array(hrtime(HRTIME_AS_ARRAY));
  ensure_array(hrtime());

  $int_time = hrtime(true);
  ensure_int($int_time);

  $array_time = hrtime();
  ensure_array($array_time);
  $array_time = hrtime(false);
  ensure_array($array_time);

  $as_int = [false, true];
  foreach ($as_int as $arg) {
    $mixed_time = hrtime($arg);
    var_dump(gettype($mixed_time));
  }
}

test();
