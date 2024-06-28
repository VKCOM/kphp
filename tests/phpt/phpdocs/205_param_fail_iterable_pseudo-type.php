@kphp_should_fail
\Iterable type isn't supported\
<?php
/**@param iterable $it*/
function foo($it) {
    var_dump($it);
}
$arr = [1,2,3];
foo($arr);
