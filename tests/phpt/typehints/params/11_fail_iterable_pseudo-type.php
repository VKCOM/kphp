@kphp_should_fail
\Iterable type isn't supported\
<?php
function foo(iterable $it) {
    var_dump($it);
}
$arr = [1,2,3];
foo($arr);
