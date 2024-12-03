@ok
<?php

function f(int $a, string $b, float $c = 0.0) {
    echo "f: a $a b $b c $c\n";
}

class A {
    const MIXED_13 = [13, 's13', 13.13];

    function g(int $a, string $b, ...$rest) {
        $n = count($rest);
        $first = $n ? $rest[0] : '';
        $last = $n ? $rest[$n-1] : '';
        echo "g: a $a, b $b, rest $n $first..$last\n";
    }
}

$zero = 0;
f(1, 's1');
f(...[2, 's2']);
f(...[3, 's3', 3.3+$zero]);
f(4, ...['s4']);
f(5, ...['s5', 5.5]);
f(6, 's6', ...[]);
f(7, 's7', ...[7.7]);
f(...[...[8, 's8'], ...[8.8]]);
f(...[9], ...['s9']);
f(...[], ...[10+0, 's10'.''], ...[], ...[10.10]);
f(...[...[...[11, 's11']]]);
f(...[12+0+$zero], ...[...[]], ...[...[...['s12']]]);
f(...A::MIXED_13);
// PHP does not allow positional arguments after unpacking (fails even in if(0)), but KPHP does
// f(...[14], 's14', ...[14.14]);

$a = new A;
$a->g(...[1, 's1']);
$a->g(...[2, 's2'], ...['x2', 'y2'], ...['z2']);
$a->g(...[3, 's3', 'x3', 'y3']);
$a->g(4, ...['s4', 'x4']);
$f5 = 5;
$y5 = 'y5';
$w5 = 'w5';
$a->g($f5, ...['s5', ...['x5', ...[$y5]]], ...['z5'], ...[], ...[$w5]);

function callL(...$rest) {
    $l = function(int $a, string $b, ...$rest) {
        $n = count($rest);
        $first = $n ? $rest[0] : '';
        $last = $n ? $rest[$n-1] : '';
        echo "l: a $a, b $b, rest $n $first..$last\n";
    };
    $l(...[10, 's10', ...$rest]);
    $l(...[11, 's11'], ...$rest);
    $l(...[12, 's12', 'p12'], ...$rest);
    $l(13, ...['s13'], ...['p13'], ...$rest, ...['z14']);
    $l(14, ...['s14'], ...['p14', ...$rest, 'z14']);
}

callL('x', 'y');

// todo unpack constants ...A::CONST_ARR

