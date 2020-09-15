@kphp_should_fail
/Expected a var name, ref or list, found 42/
<?php

$xs = [1];
foreach ($xs as 42 => $_) {}
