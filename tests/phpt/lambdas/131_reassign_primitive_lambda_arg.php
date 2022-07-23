@kphp_should_fail
/\$value = getMixedArr\(\);/
/\$value is was declared as @param string, and later reassigned to mixed\[\]/
/Please, use different names for local vars and arguments/
<?php

/** @return mixed[] */
function getMixedArr() {
    return [];
}

// in lambdas (unlike regular functions), assumptions for arguments are saved even for primitives
// that's why KPHP analyzes reassigns to them, and they can't be easily splitted
// (assumptions for primitives help to infer typed callables in generics related to primitives)
$f = function(string $value) {
    $value = getMixedArr();
};
$f();
