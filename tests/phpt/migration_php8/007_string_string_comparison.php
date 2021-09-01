@kphp_runtime_should_warn
/String is float result in PHP 7 and PHP 8 are different for '100 ' \(PHP7: false, PHP8: true\)/
/String is float result in PHP 7 and PHP 8 are different for '100 ' \(PHP7: false, PHP8: true\)/
<?php

$normal = "100";
$leading = " 100";
$trailing = "100 ";

set_migration_php8_warning(0b010);

var_dump($normal == $normal);
var_dump($normal == $leading); // warning
var_dump($normal == $trailing);

var_dump($normal < $normal);
var_dump($normal < $leading); // warning
var_dump($normal < $trailing);
