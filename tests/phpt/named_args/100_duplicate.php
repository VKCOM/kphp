@kphp_should_fail
/Named arguments with duplicated name: 'a'/
<?php


function test($a, $b) {}

test(a: 1, a: 2);
