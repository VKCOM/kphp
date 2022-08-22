@kphp_runtime_should_warn
/DateTime::modify\(\): Failed to parse time string \(\+1 day \+four month\) at position 7 \(\+\): Unexpected character/
<?php

function test_modify_wrong_modifier() {
  $date = new DateTime('2000-12-31');
  $date->modify('+1 day +four month');
}

test_modify_wrong_modifier();
