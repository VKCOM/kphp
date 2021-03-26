@ok
<?php

function arg(string $tag, int $value): int {
  echo "$tag:$value\n";
  return $value;
}

function test_keys() {
  $array1 = array(arg('key', 10) => arg('val', 20));
  var_dump($array1);

  $array2 = array(
    arg('key1', 10) => array(
      arg('val1', 2),
      arg('val2', 3),
      arg('val3', 4),
      arg('val4', 5),
      arg('val5', 6),
      arg('val6', 7),
      arg('val7', 8),
      arg('val8', 9),
    ),
    arg('key2', 20) => arg('val9', -10),
  );
  var_dump($array2);
}

function test_keys_short() {
  $array1 = [arg('key', 10) => arg('val', 20)];
  var_dump($array1);
}

function test_values() {
  // Less than 2 impure elements - evaluation order doesn't matter
  $array1 = array(arg('x1', 1), 2, 3);
  var_dump($array1);
  $array2 = array(1, 2, arg('x2', 3));
  var_dump($array2);

  // More than 1 impure elements - evaluation order should be enforced
  $array3 = array(1, arg('x', 2), 3, arg('y', 4));
  var_dump($array3);
  $array4 = array(arg('a', 1), arg('b', 2), arg('c', 3));
  var_dump($array4);

  $nested = array(
    array(arg('nested1', 1), arg('nested2', 2)),
    array(arg('nested3', 3)),
    arg('nested4', 4),
    arg('nested5', 5)
  );
  var_dump($nested);

  // More arguments that we can wrap into a macro.
  $many_values = [
    arg('el1', 1),
    arg('el2', 2),
    arg('el3', 3),
    arg('el4', 4),
    arg('el5', 5),
    arg('el6', 6),
    arg('el7', 7),
    arg('el8', 8),
    arg('el9', 9),
    arg('el10', 10),
  ];
  var_dump($many_values);
}

function test_values_short() {
  $array1 = [arg('x1', 1), 2, 3];
  var_dump($array1);
  $array2 = [1, 2, arg('x2', 3)];
  var_dump($array2);

  $array3 = [1, arg('x', 2), 3, arg('y', 4)];
  var_dump($array3);
  $array4 = [arg('a', 1), arg('b', 2), arg('c', 3)];
  var_dump($array4);

  $nested = array(
    [arg('nested1', 1), arg('nested2', 2)],
    [arg('nested3', 3)],
    arg('nested4', 4),
    arg('nested5', 5)
  );
  var_dump($nested);
}

test_keys();
test_keys_short();
test_values();
test_values_short();
