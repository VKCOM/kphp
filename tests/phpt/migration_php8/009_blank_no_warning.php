@ok k2_skip kphp_runtime_should_not_warn
<?php

require_once 'kphp_tester_include.php';

set_migration_php8_warning(0b011);

$blank = " ";
$blank_more = "   \t";

var_dump(is_numeric($blank));
var_dump(is_numeric($blank_more));

$blank = " ";
$blank_more = "  \t";
$normal = "100";

var_dump($blank == $normal);
var_dump($blank_more == $normal);
var_dump($normal >= '0');
