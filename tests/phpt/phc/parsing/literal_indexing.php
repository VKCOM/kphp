@ok
<?php

const STR_CONST = 'strconst';
const ARR_CONST = [1, 2];

function string_indexing1() {
  var_dump('a'[0]);
  var_dump("b"[0]);
  var_dump("bar"[2]);

  var_dump('a' . 'b'[0]);
  var_dump(('a' . 'b')[1]);

  var_dump(('abc')[1]);

  var_dump((STR_CONST)[0]);
}

function array_indexing1() {
  var_dump(array('a')[0]);
  var_dump(['a'][0]);

  var_dump((array('a'))[0]);
  var_dump((['a'])[0]);

  var_dump(['a' => ['b' => 10]]['a']['b']);
  var_dump((['a' => ['b' => 10]]['a'])['b']);

  // non-const arrays
  $x = 10;
  var_dump(array($x => 'foo')[10]);
  var_dump([$x => 'foo'][10]);
  var_dump([$x => 'foo'][$x]);
  var_dump(([$x => 'foo'])[$x]);
  var_dump([$x => [$x + 1 => 'ok']][$x][$x+1]);
  var_dump([$x => [$x + 1 => 'ok']][$x][$x+1][0]);
  var_dump([$x => [$x + 1 => 'ok']][$x][$x+1][1]);

  var_dump([
    $x === 0 => 'a',
    $x === 5 => 'b',
    $x === 10 => 'c',
  ][true]);

  var_dump([
    $x === 5 => 'b',
    $x === 10 => 'c',
    $x === 0 => 'a',
  ][true]);

  var_dump((ARR_CONST)[0]);
}

function array_indexing2() {
  var_dump([1, 2, 3, 4,][3]);
  var_dump([array(1,2,3), [4, 5, 6]][1][2]);
  foreach (array([1, 2, 3])[0] as $v) {
    var_dump($v);
  }

  var_dump(array(1, 2, 3, 4,) [3]);
  var_dump(array(array(1,2,3), array(4, 5, 6))[1][2]);
  foreach (array(array(1, 2, 3))[0] as $v) {
    var_dump($v);
  }
}

string_indexing1();
array_indexing1();
array_indexing2();
