@ok k2_skip
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

/**
 * @param mixed $p1
 * @param ?mixed $p4
 */
function appendref($ref, $p1, string $p2, ?string $p3, $p4) {
  $ref .= $p1 . $p2 . $p3;
  var_dump($ref);
  $ref .= $p1 . $p2 . $p3 . $p4;
  var_dump($ref);
}

/**
 * @param mixed $p1
 * @param ?mixed $p4
 */
function append_test($p1, string $p2, ?string $p3, $p4) {
  $s = 'a';
  $s .= $p1 . $p2 . $p3 . $p4;
  var_dump($s);
  $s .= $p1 . $p4;
  var_dump($s);

  /** @var ?string $optional_s */
  $optional_s = 'a';
  $optional_s .= $p1;
  var_dump($optional_s);
  $optional_s .= $p1 . $p2;
  var_dump($optional_s);
  $optional_s .= $p1 . $p2 . $p3 . $p4;
  var_dump($optional_s);

  /** @var string|false $orfalse_s */
  $orfalse_s = false;
  $orfalse_s .= $p1 . $p2 . $p3;
  var_dump($orfalse_s);
  $orfalse_s .= $p1 . $p2 . $p3 . $p4;
  var_dump($orfalse_s);

  /** @var string|null $ornull_s */
  $ornull_s = null;
  $ornull_s .= $p1 . $p2 . $p3;
  var_dump($ornull_s);
  $ornull_s .= $p1 . $p2 . $p3 . $p4;
  var_dump($ornull_s);

  $i = 535;
  $i .= $p1 . $p2 . $p3;
  var_dump($i);

  /** @var mixed $s2 */
  $s2 = 42;
  if (true) {
    $s2 = 'a';
  }
  $s2 .= $p1 . $p2 . $p3;
  var_dump($s2);

  if (true) {
    $s3 = get_mixed();
  }
  if ($s3) {
    $s3 .= "a" . $p2 . "b";
    var_dump($s3);
  }

  $s4 = '';
  if (true) {
    $s4 = get_mixed();
  }
  if ($s4) {
    $s4 .= $p1 . $p2 . $p4;
    var_dump($s4);
  }

  if ($p1) {
    $s5 .= "a" . $p1 . "b";
    var_dump($s5);
  }

  $s6 = 'a';
  $s6 .= $s6 . $s6;
  var_dump($s6);
  $s6 .= $s6 . $s6 . $s6 . $s6;
  var_dump($s6);
}

function append_test2(string $x, string $y) {
  $s = 545;
  var_dump($s);
  $s = $x . $y;
  $s .= $x . $y;
  var_dump($s);
}

/** @return mixed */
function get_mixed() { return 'a'; }

function append4arr($parts) {
  $s = '';
  $s .= $parts[0] . $parts[1] . $parts[2] . $parts[3];
  var_dump($s);
}

function append4complex($parts) {
  /** @var string[] $lhs */
  $lhs = ['a'];
  $lhs[0] .= $parts[0] . $parts[1] . $parts[2] . $parts[3];
  var_dump($lhs);

  $v = $parts[0];
  /** @var string[] $arr */
  $arr = [];
  $arr['k'] = '';
  $arr['k'] .= "<$v>";
  var_dump($arr['k']);
  $arr['k'] .= "<$v/$v>";
  var_dump($arr['k']);
  $arr['k'] .= "<$v/$v>" . $parts[0] . $parts[1];
  var_dump($arr['k']);

  $wrapper = new StringWrapper();
  $wrapper->str = (string)$parts[0];
  /** @var string[] $arr2 */
  $arr2 = [];
  $arr2['k'] = '';
  $arr2['k'] .= "<$wrapper>";
  var_dump($arr2);

  $i = 32;
  /** @var string[] $arr3 */
  $arr3 = [];
  $arr3['k'] .= "<$i>";
  $arr3['k'] .= "<$i>" . ">$i<";
  var_dump($arr3);
}

function append4complex2($parts) {
  $lhs = new StringWrapper();
  $lhs->str = 'fd';
  $lhs->str .= $parts[0] . $parts[1] . $parts[2] . $parts[3];
  var_dump($lhs->str);

  $lhs2 = new StringWrapper2('example');
  $lhs2->wrapper->str .= 'extra';
  var_dump($lhs2->wrapper->str);
  $lhs2->wrapper->str .= $parts[0] . $parts[1] . $parts[2];
  var_dump($lhs2->wrapper->str);
}

function append_static($parts) {
  static $static_s = '';
  $static_s .= $parts[0] . $parts[1];
  var_dump($static_s);
  $static_s .= $parts[0] . $parts[1] . $parts[2] . $static_s;
  var_dump($static_s);
  return $static_s;
}

function test_md5_1() {
  // 11 chars
  $a = '11111111111';
  $b = '11111111111';
  $c = '11111111111';
  $b .= substr(md5($a . $b . $c), 0, 10);
  var_dump($b);
}

function test_md5_2() {
  $a = '11111111111';
  $b = '11111111111';
  $c = '11111111111';
  $b = $b . substr(md5($a . $b . $c), 0, 10);
  var_dump($b);
}

function test_md5_3() {
  // 12 chars
  $a = '111111111111';
  $b = '111111111111';
  $c = '111111111111';
  $b .= substr(md5($a . $b . $c), 0, 10);
  var_dump($b);
}

function tmp_expr_safe_set_value() {
  $a = ['aaa'];
  $a['b'] = substr($a['b'], 0, 1);
  var_dump($a);
  $a[substr($a['a'], 0, 1)] = 'aaa';
  var_dump($a);
}

function tmp_expr_safe_set_op1() {
  $a = 'aaa';
  $a = $a . substr($a, 0, 1);
  var_dump($a);
}

function tmp_expr_safe_set_op2() {
  $a = 'aaa';
  $a = substr($a, 0, 1);
  var_dump($a);
}

function tmp_expr_safe_push_back() {
  $a = ['aaa'];
  $a[] = substr($a[0], 0, 1);
  var_dump($a);
}

function tmp_expr_safe_push_back_return() {
  $a = ['aaa'];
  var_dump($a[] = substr($a[0], 0, 1));
  var_dump($a);
}

function tmp_expr_safe_index() {
  $a = ['a' => 'aaa', 'aa' => 'bbb'];
  var_dump($a[substr($a['a'], 0, 1)]);
  var_dump($a['a' . substr($a['a'], 0, 1)]);
}

function tmp_concat() {
  $a = 'aaaa';
  $x = substr($a, 0, 1) . substr($a, 0, 1) . substr($a, 0, 1);
  var_dump($x);
}

class StringWrapper {
  public string $str = '';
  public function __toString(): string {
    return $this->str;
  }
}

class StringWrapper2 {
  public StringWrapper $wrapper;
  public function __construct(string $s) {
    $this->wrapper = new StringWrapper();
    $this->wrapper->str = $s;
  }
}

$empty = '';
$conststr = 'hello global';
$nonconst = modify1($empty);

$str_parts = [
  $conststr,
  'hello world',
  $empty,
  $nonconst,
];

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

  append_test($nonconst, $empty, $empty, $str_parts[3]);
  append_test($nonconst, (string)$nonconst, (string)$nonconst, (string)$nonconst);
  append_test($str_parts[0], (string)$str_parts[1], (string)$str_parts[2], $str_parts[3]);
  append_test2($empty, (string)$str_parts[0]);
  append_test2((string)$str_parts[0], (string)$str_parts[1]);
  append_test2((string)$str_parts[2], (string)$str_parts[3]);
  append4arr($str_parts);
  append4complex($str_parts);
  append4complex2($str_parts);
  var_dump(append_static($str_parts));

  $s_ref = 'a';
  appendref($s_ref, $str_parts[0], (string)$str_parts[1], (string)$str_parts[2], $str_parts[3]);
  var_dump($s_ref);
  appendref($s_ref, $str_parts[0], (string)$str_parts[1], (string)$str_parts[2], $str_parts[3]);
  var_dump($s_ref);

  test_md5_1();
  test_md5_2();
  test_md5_3();

  tmp_expr_safe_set_value();
  tmp_expr_safe_set_op1();
  tmp_expr_safe_set_op2();
  tmp_expr_safe_push_back();
  tmp_expr_safe_push_back_return();
  tmp_expr_safe_index();
  tmp_concat();
}
