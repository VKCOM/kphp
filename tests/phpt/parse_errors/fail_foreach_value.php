@kphp_should_fail
/Expected a var name, ref or list, found foo/
<?php

$xs = [1];
foreach ($xs as foo()) {}
