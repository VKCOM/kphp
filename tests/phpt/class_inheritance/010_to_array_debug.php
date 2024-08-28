@ok k2_skip
<?php

require_once '008_two_level_inheritance.php';
require_once 'kphp_tester_include.php';

var_dump(to_array_debug($d));

class EmptyClass { function foo() {} }
var_dump(to_array_debug(new EmptyClass()));
