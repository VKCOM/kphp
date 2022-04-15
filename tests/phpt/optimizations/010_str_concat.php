@ok
<?php

// Test that we don't break anything when returning one of the operands without explicit
// copy when optimizing empty string concat operand.
// We rely on copy-on-write semantics, the reference counter will increase and
// copy will not be created until one of the strings will be modified.

function modify1($empty) {
  $s = 'hello';
  $s2 = $s . $empty;
  $s2[0] = 'a';
  var_dump($s, $s2);
  return $s2;
}

function modify2($empty) {
  $s = 'hello2';
  $s2 = $empty . $s;
  $s2[0] = 'a';
  var_dump($s, $s2);
}

function modify3() {
  $s = 'hello3';
  $s2 = '' . $s;
  $s2[0] = 'a';
  var_dump($s, $s2);
}

function modify4($nonconst) {
  $s2 = $nonconst . '';
  $s2[0] = 'a';
  var_dump($nonconst, $s2);
}

function modify5($nonconst, $empty) {
  $s2 = $nonconst . $empty;
  $s2[0] = 'a';
  var_dump($nonconst, $s2);
}

function modify6($s) {
  $s2 = $s . '';
  $s2[0] = 'a';
  var_dump($s, $s2);
}

function modify7($s, $empty) {
  $s2 = $empty . $s;
  $s[0] = 'a';
  var_dump($s, $s2);
}

function modify8($s, $empty) {
  $s2 = $empty . $s;
  $s[0] = 'b';
  var_dump($s, $s2);
}

function modify9(&$s, $empty) {
  $s2 = $empty . $s;
  $s[0] = '?';
  var_dump($s, $s2);
}

$empty = '';
$conststr = 'hello global';
$nonconst = modify1($empty);

for ($i = 0; $i < 2; $i++) {
  modify1($empty);
  modify2($empty);
  modify3();
  modify4($nonconst);
  modify4($empty);
  modify5($nonconst, $empty);
  modify5($empty, $empty);
  modify6($conststr);
  modify6($empty);
  modify7($nonconst, $empty);
  modify8($nonconst, $empty);

  modify9($nonconst, $empty);
  modify9($nonconst, $empty);
}
