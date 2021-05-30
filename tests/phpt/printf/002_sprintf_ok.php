@ok
<?php

function nullableString(): ?string {
    return null;
}

/**
 * @return string|false
 */
function orFalseString(): string {
    return false;
}

$int = 1;
$float = 1.1;
$string = "Hello";
$string = "Hello";

// simple
echo sprintf("%d", $int);
echo sprintf("%X", $int);
echo sprintf("%f", $float);
echo sprintf("%d%f", $int, $float);
echo sprintf("%d or %f", $int, $float);
echo sprintf("%c or %f", $int, $float);
echo sprintf("%c or %g", $int, $float);

// with arg num
echo sprintf('%2$d', $string, $int, $float);
echo sprintf('some %2$d', $string, $int, $float);
echo sprintf('%3$f', $string, $int, $float);
echo sprintf('%s %3$f %d', $string, $int, $float);
echo sprintf('%s %3$f %3$f %d some %f %1$s', $string, $int, $float);
echo sprintf('%s%3$f %d%2$d %f%1$s', $string, $int, $float);
echo sprintf('%\'#s%3$f %0d%2$10.5d %.5f%1$-s', $string, $int, $float);
echo sprintf('%s%3$f %d%2$d %f%1$s with %s %3$f some %3$f %d %f %1$s text', $string, $int, $float, $string, $int, $float);

// nullable arg
echo sprintf('%s', nullableString() ?? "");
