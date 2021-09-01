@kphp_runtime_should_warn

<?php

$leading = " 100";
$trailing = "100 ";
$normal = "100";

set_migration_php8_warning(0b010);

var_dump(is_numeric($leading));
var_dump(is_numeric($trailing)); // warning
var_dump(is_numeric($normal));
