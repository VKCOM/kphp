@ok k2_skip
<?php
  $empty = array();
  var_dump(json_encode($empty));
  var_dump(json_encode($empty, JSON_FORCE_OBJECT));
  $a = array(0, 1, 2, 3, 4, 5, 6);
  var_dump(json_encode($a));
  var_dump(json_encode($a, JSON_FORCE_OBJECT));
  unset($a[4]);
  var_dump(json_encode($a));
  var_dump(json_encode($a, JSON_FORCE_OBJECT));
