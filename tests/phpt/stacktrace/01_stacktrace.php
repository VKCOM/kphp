@kphp_should_fail
/pass int\[\] to argument \$x of acInt/
/\/\/ acInt\(\$a\);/
/\$a is int\[\]/
/\/\/ \$a = \$f;/
/\$f is int\[\]/
/\/\/ \$f = \[1\];/
/array is int\[\]/
/1 is int/
<?php
function acInt(int $x) {}
function demo() {
    $f = [1];
    $a = $f;
    acInt($a);
}

demo();
