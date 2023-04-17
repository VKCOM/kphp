@ok
<?php
require_once 'kphp_tester_include.php';

class Chain {
  public ?Chain $next = null;
}

function test_chain_objects() {
  $obj = new Chain;
  $obj->next = new Chain;
  var_dump(instance_to_array($obj));
}

function test_max_depth_level() {
  $obj = new Chain;
  $ptr = $obj;
  for ($i = 0; $i < 64; ++$i) {
    $ptr->next = new Chain;
    $ptr = $ptr->next;
  }

  // 64 lvl depth is ok
  var_dump(count(instance_to_array($obj)));

  $ptr->next = new Chain;

  // allowed depth exceeded
  var_dump(instance_to_array($obj));
}

test_chain_objects();
test_max_depth_level();
