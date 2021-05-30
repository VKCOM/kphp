@kphp_should_fail
// simple
/For format specifier '%d', type 'int' is expected, but 1 argument has type 'string'/
/For format specifier '%d', type 'int' is expected, but 1 argument has type 'string'/
/For format specifier '%d', type 'float' is expected, but 1 argument has type 'string'/
// with arg num
/For format specifier '%1$d', type 'int' is expected, but 1 argument has type 'string'/
/For format specifier '%1$d', type 'int' is expected, but 2 argument has type 'float'/
// mixed with arg num and without
/For format specifier '%f', type 'float' is expected, but 2 argument has type 'int'/
/For format specifier '%3$d', type 'int' is expected, but 3 argument has type 'float'/
// not enough, simple
/Not enough parameters for format string '%s %3$d %f', expected number of arguments: 2, passed 1/
// not enough, with arg num
/Not enough parameters for format string '%s %5$f', expected number of arguments: 5, passed 1/
// nullable arg
/For format specifier '%s', type 'string' is expected, but 1 argument has type '\?string'/
// or false arg
/For format specifier '%s', type 'string' is expected, but 1 argument has type 'string|false'/
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
sprintf("%d", $string);
sprintf("%X", $string);
sprintf("%f", $string);

// with arg num
sprintf('%1$d', $string, $int, $float);
sprintf('%3$d', $string, $int, $float);

// mixed with arg num and without
sprintf('%s %3$d %f', $string, $int, $float);

// not enough, simple
sprintf('%s %f', $string);

// not enough, with arg num
sprintf('%s %5$f', $string);

// nullable arg
sprintf('%s', nullableString());

// or false arg
sprintf('%s', orFalseString());
