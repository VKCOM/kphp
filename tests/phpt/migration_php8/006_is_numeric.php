@kphp_runtime_should_warn
/is_numeric\('100 '\) result in PHP 7 and PHP 8 are different \(PHP7: false, PHP8: true\)/
/is_numeric\('100 \t\t'\) result in PHP 7 and PHP 8 are different \(PHP7: false, PHP8: true\)/
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

var_dump(is_numeric("0x123"));
var_dump(is_numeric("inf"));
var_dump(is_numeric("inFinItY"));
var_dump(is_numeric("NaN"));
var_dump(is_numeric(" +0x123"));
