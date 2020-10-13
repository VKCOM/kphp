@ok
<?php

/**
 * @kphp-infer cast
 * @param string $name
 */
function printName($name) {
  echo $name, "\n";
}

// this is mixed[]
$user = ['id' => 1, 'name' => 'Vasya'];
// but calling printName(mixed) is ok because of cast
// (we just call it with apriopi runtime-string argument)
printName($user['name']);

// even this is ok occasionally, as in PHP echo (number) and echo (string) are equal, in KPHP number will be casted
printName(123);
// (but this will work differently:)
// printName([1,2,3]);
