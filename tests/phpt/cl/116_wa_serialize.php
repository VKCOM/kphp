@ok
<?php
require_once 'kphp_tester_include.php';
require_once 'kphp_tester_include.php';

$a = new Classes\A();
echo serialize(instance_to_array($a));
