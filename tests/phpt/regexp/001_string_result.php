@ok k2_skip
<?php

function accept_string(string $s) { var_dump($s); }

/** @param ?string $s */
function accept_nullable_string($s) { var_dump($s); }

/** @param string|null|false $s */
function accept_string_or_null_or_false($s) { var_dump($s); }

function test_preg_replace1(string $subject) {
  $result = preg_replace('/_/', 'x', $subject);
  accept_string_or_null_or_false($result);
  if ($result) {
    accept_string($result);
  }
  if ($result) {
    accept_string($result);
  }
}

function test_preg_replace2(string $subject) {
  $pattern = '/_/';
  $result = preg_replace($pattern, 'x', $subject);
  accept_string_or_null_or_false($result);
  if ($result) {
    accept_string($result);
  }
  if ($result) {
    accept_string($result);
  }
}

/** @return mixed */
function as_mixed($x) { return $x; }

/**
 * @kphp-infer
 * @param mixed $name
 * @return string
 */
function test_preg_replace3($name) {
  $name = strtolower(as_mixed($name));
  $name = preg_replace('/\s/', '-', $name);
  $name = preg_replace('/[^a-z0-9-]/', '', $name);
  if ($name) {
    return $name;
  }
  return '';
}

function test_preg_replace4(string $text) {
  $patterns = ['/_/', '/\s/'];
  $result = preg_replace($patterns, '', $text);
  accept_string_or_null_or_false($result);
  if ($result) {
    accept_string($result);
  }
  if ($result) {
    accept_string($result);
  }
}

/** @param mixed $replace */
function test_preg_replace5(string $subject, $replace) {
  $result = preg_replace('/_/', $replace, $subject);
  accept_string_or_null_or_false($result);
  if ($result) {
    accept_string($result);
  }
  if ($result) {
    accept_string($result);
  }
}

function test_preg_replace_callback1(string $subject) {
  $result = preg_replace_callback('/_/', function ($m) {
    return '{' . $m[0] . '}';
  }, $subject);
  accept_nullable_string($result);
  if ($result !== null) {
    accept_string($result);
  }
  if ($result) {
    accept_string($result);
  }
}

function test_preg_replace_callback2(string $subject) {
  $pattern = '/_/';
  $result = preg_replace_callback($pattern, function ($m) {
    return '{' . $m[0] . '}';
  }, $subject);
  accept_nullable_string($result);
  if ($result !== null) {
    accept_string($result);
  }
  if ($result) {
    accept_string($result);
  }
}

/**
 * @kphp-infer
 * @param mixed $name
 * @return string
 */
function test_preg_replace_callback3($name) {
  $name = strtolower(as_mixed($name));
  $name = preg_replace_callback('/\s/', function ($m) { return '-'; }, $name);
  $name = preg_replace_callback('/[^a-z0-9-]/', function ($m) { return ''; }, $name);
  if ($name !== null) {
    return $name;
  }
  return '';
}

function test_preg_replace_callback4(string $text) {
  $patterns = ['/_/', '/\s/'];
  $result = preg_replace_callback($patterns, function ($m) { return ''; }, $text);
  accept_nullable_string($result);
  if ($result !== null) {
    accept_string($result);
  }
  if ($result) {
    accept_string($result);
  }
}

test_preg_replace1('hello_world');
test_preg_replace1('foo');
test_preg_replace2('hello_world');
test_preg_replace2('foo');
test_preg_replace3('foo @() -- 12');
test_preg_replace4('hello _world_ ');
test_preg_replace4('foo');
test_preg_replace5('hello_world', '_');
test_preg_replace5('foo', '_');

test_preg_replace_callback1('hello_world');
test_preg_replace_callback1('foo');
test_preg_replace_callback2('hello_world');
test_preg_replace_callback2('foo');
test_preg_replace_callback3('foo @() -- 12');
test_preg_replace_callback4('hello _world_ ');
test_preg_replace_callback4('foo');
