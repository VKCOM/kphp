@ok
<?php
require_once 'kphp_tester_include.php';

$a1 = new Classes\A();
$a2 = 8;
$a3 = array(new Classes\A);

if (is_object($a1))
  echo get_class($a1), "\n";
else
  echo "a1 not object\n";

if (is_object($a2))
  echo get_class($a2), "\n";
else
  echo "a2 not object\n";

if (is_object($a3))
  echo get_class($a3), "\n";
else
  echo "a3 not object\n";
