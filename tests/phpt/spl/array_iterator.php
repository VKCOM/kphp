@ok k2_skip
<?php

function test_empty_array_iterator() {
  $it = new ArrayIterator([]);
  var_dump($it->current());
  var_dump($it->key());
  var_dump($it->count());
  var_dump($it->valid());
  $it->next();
  var_dump($it->current());
  var_dump($it->key());
  var_dump($it->count());
  var_dump($it->valid());
}

function test_invalid_array_iterator() {
  $it = new ArrayIterator([1]);
  var_dump($it->valid());
  $it->next();
  var_dump($it->valid());
  var_dump($it->key());
  var_dump($it->current());

  var_dump($it->valid());
  $it->next();
  $it->next();
  var_dump($it->valid());
  var_dump($it->key());
  var_dump($it->current());
}

/** @param ArrayIterator $iter */
function test1($iter) {
  $i = 0;
  while ($iter->valid()) {
    var_dump("$i => " . json_encode($iter->current()) . '/' . $iter->key());
    $iter->next();
  }
}

/** @param ArrayIterator $iter */
function test2($iter) {
  if (!$iter->valid()) {
    return;
  }
  for ($i = 0; $i < $iter->count(); $i++) {
    var_dump("$i => " . json_encode($iter->current()) . '/' . $iter->key());
    $iter->next();
  }
}

$arrays = [
  [],
  [''],
  [0],
  [1, 2, 3],
  ['a' => 1, 'b' => 2],
  [5 => 1, 'x' => 53, 'b' => 'str', 0 => 42],
  [10 => 'a', 2 => 'b'],
  [[], []],
  [[1], [2]],
  ['a' => [1], 'b' => [2, 3]],
];

test_empty_array_iterator();
test_invalid_array_iterator();

foreach ($arrays as $a) {
  test1(new ArrayIterator($a));
  $iter = new ArrayIterator($a);
  var_dump($iter->valid());
  test1($iter);
  var_dump($iter->valid());

  $array_copy = $a;
  $iter = new ArrayIterator($array_copy);
  var_dump($iter->count());
  if (count($array_copy) !== 0) {
    array_pop($array_copy);
  }
  var_dump($iter->count());
  test1($iter);
  var_dump($iter->count());

  $iter = new ArrayIterator($array_copy);
  test2($iter);
  test1($iter);
  $iter = new ArrayIterator($array_copy);
  test1($iter);
  test2($iter);
}
