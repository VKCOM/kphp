@kphp_should_fail
/pass array< int > to argument \$x of acInt/
/\/\/ acInt\(\$a\);/
/\$a is array< int >/
/\/\/ \$a = \$f;/
/\$f is array< int >/
/\/\/ \$f = \[1\];/
/array is array< int >/
/1 is int/
<?php
function acInt(int $x) {}
function demo() {
    $f = [1];
    $a = $f;
    acInt($a);
}

demo();
