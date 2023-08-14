@kphp_should_fail
\Iterable type isn't supported\
<?php
function foo (iterable $it1  =  [ ], iterable $it2  =  null)  { // iterable may use null or an array as a default value
    var_dump($it1);
    var_dump($it2);
}
$arr1 = [];
$arr2 = null;
foo($arr1, $arr2);
