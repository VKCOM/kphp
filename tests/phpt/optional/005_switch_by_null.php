@ok
<?php

/**
 * @param int|false|null $x
 */
function test_switch_by_optional_int($x) {
  switch ($x) {
    case null:
      echo "null\n";
      break;
    case false:
      echo "false\n";
      break;
    case 0:
      echo "0\n";
      break;
    default:
      echo "default\n";
      break;
  }
}

/**
 * @param false|null $x
 */
function test_switch_by_optional_false($x) {
  switch ($x) {
    case null:
      echo "null\n";
      break;
    case false:
      echo "false\n";
      break;
    default:
      echo "default\n";
      break;
  }
}

/**
 * @param boolean|null $x
 */
function test_switch_by_optional_bool($x) {
  switch ($x) {
    case null:
      echo "null\n";
      break;
    case false:
      echo "false\n";
      break;
    case true:
      echo "true\n";
      break;
    default:
      echo "default\n";
      break;
  }
}


test_switch_by_optional_int(null);
test_switch_by_optional_int(false);
test_switch_by_optional_int(0);
test_switch_by_optional_int(1);

test_switch_by_optional_false(null);
test_switch_by_optional_false(false);

test_switch_by_optional_bool(null);
test_switch_by_optional_bool(false);
test_switch_by_optional_bool(true);
