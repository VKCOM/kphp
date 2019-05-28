@kphp_should_fail
<?php

#ifndef KittenPHP
function tuple(...$args) { return $args; }
#endif

$a = tuple(1);
var_dump($a[0b10]);

