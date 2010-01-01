@ok
<?php
  $test = '1:2:3';
  $gift = array();
  list($gift['from_id'], $gift['gift_number'], $gift['id']) = explode(':', $test);

  var_dump (explode(':', $test));
  var_dump ($gift);

  $a = array (2 => 1, 3 => true);
  list ($a[-1], $a[0], $a[1]) = $a;
  var_dump ($a);
