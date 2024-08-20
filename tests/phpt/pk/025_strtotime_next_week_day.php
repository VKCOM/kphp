@ok k2_skip
<?php

for ($shift = 0; $shift < 7; $shift++) {
  foreach (["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"] as $x) {
    var_dump(date("l Ymd", strtotime("next $x", 1573469190 + 86400 * $shift)));
    var_dump(date("l Ymd", strtotime("last $x", 1573469190 + 86400 * $shift)));
  }
  echo "======\n";
}
