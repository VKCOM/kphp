@ok k2_skip
<?php
require_once 'kphp_tester_include.php';
require_once 'kphp_tester_include.php';

$a = new Classes\A();
echo serialize(to_array_debug($a));
