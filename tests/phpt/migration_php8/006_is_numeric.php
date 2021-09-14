@kphp_runtime_should_warn
/String is float result in PHP 7 and PHP 8 are different for '100 ' \(PHP7: false, PHP8: true\)/
/String is float result in PHP 7 and PHP 8 are different for '100 \t\t' \(PHP7: false, PHP8: true\)/
<?php

$leading = " 100";
$trailing = "100 ";
$trailing_tab = "100 \t\t";
$trailing_not_space = "100 f";
$normal = "100";

set_migration_php8_warning(0b010);

var_dump(is_numeric($leading));
var_dump(is_numeric($trailing)); // warning
var_dump(is_numeric($trailing_tab)); // warning
var_dump(is_numeric($trailing_not_space));
var_dump(is_numeric($normal));
