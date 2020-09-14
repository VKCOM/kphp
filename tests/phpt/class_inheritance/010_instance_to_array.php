@ok
<?php

require_once '008_two_level_inheritance.php';
require_once 'kphp_tester_include.php';

var_dump(instance_to_array($d));

class EmptyClass { function foo() {} }
var_dump(instance_to_array(new EmptyClass()));
