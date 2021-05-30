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

function getString(): string {
    return "";
}

$int = 1;
$float = 1.1;
$string = "Hello";
$string = "Hello";

// simple
vprintf("%d", [$int]);
vprintf("%s", [getString()]);
vprintf("%X", [$int]);
vprintf("%f", [$float]);
vprintf("%d%f", [$int, $float]);
vprintf("%d or %f", [$int, $float]);
vprintf("%c or %f", [$int, $float]);
vprintf("%c or %g", [$int, $float]);

// with arg num
vprintf('%2$d', [$string, $int, $float]);
vprintf('some %2$d', [getString(), $int, $float]);
vprintf('%3$f', [$string, $int, $float]);
vprintf('%s %3$f %d', [$string, $int, $float]);
vprintf('%s %3$f %3$f %d some %f %1$s', [getString(), $int, $float]);
vprintf('%s%3$f %d%2$d %f%1$s', [$string, $int, $float]);
vprintf('%\'#s%3$f %0d%2$10.5d %.5f%1$-s', [getString(), $int, $float]);
vprintf('%s%3$f %d%2$d %f%1$s with %s %3$f some %3$f %d %f %1$s текст', [$string, $int, $float, getString(), $int, $float]);

// nullable arg
vprintf('%s', [nullableString() ?? ""]);

// with non-const format
$format = "%s";
vprintf($format, [$int]);
vprintf($format, [$int, getString()]);

// with constant
const FORMAT = "%d%s";
vprintf(FORMAT, [$int, getString()]);

// with non-const args
$args = [$int];
vprintf('%s', $args);
