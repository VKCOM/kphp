@kphp_should_fail
/Expected a var name, ref or list as a foreach value, found function/
<?php

$xs = [1];
foreach ($xs as $_ => function() {}) {}
