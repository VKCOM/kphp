@ok
<?php

/** @return string|false */
function get_random_bytes($num)  {
#ifndef KPHP
  try {
    $res = openssl_random_pseudo_bytes($num);
  } catch (\Throwable $e) {
    return false;
  }
  return $res;
#endif
  return openssl_random_pseudo_bytes($num);
}

function test_valid_sizes() {
    $r1 = get_random_bytes(1);
    var_dump($r1 !== false && strlen($r1) === 1);

    $r16 = get_random_bytes(16);
    var_dump($r16 !== false && strlen($r16) === 16);

    $r64 = get_random_bytes(64);
    var_dump($r64 !== false && strlen($r64) === 64);

    $r256 = get_random_bytes(256);
    var_dump($r256 !== false && strlen($r256) === 256);

    $r1024 = get_random_bytes(1024);
    var_dump($r1024 !== false && strlen($r1024) === 1024);
}

function test_invalid_sizes() {
    var_dump(get_random_bytes(0) === false);
    var_dump(get_random_bytes(-1) === false);
    var_dump(get_random_bytes(-100) === false);
    var_dump(get_random_bytes(-9999) === false);
}

function test_uniqueness() {
    $r1 = get_random_bytes(16);
    $r2 = get_random_bytes(16);
    var_dump($r1 !== $r2);

    $r3 = get_random_bytes(16);
    var_dump($r3 !== $r1);
    var_dump($r3 !== $r2);
}

function test_byte_distribution() {
    $r = get_random_bytes(32);
    var_dump($r !== false);
    var_dump(strlen($r) === 32);
    var_dump(bin2hex($r) !== str_repeat('00', 32));
}

function test_repeated_uniqueness() {
    $results = [];
    for ($i = 0; $i < 20; $i++) {
        $results[] = get_random_bytes(8);
    }

    $unique = $results;
    array_unique($unique);
    var_dump(count($unique) === 20);
}

function test_binary_randomness() {
    $r = get_random_bytes(256);
    var_dump($r !== false);
    var_dump(strlen($r) === 256);
    $freq = array_count_values(str_split($r));
    var_dump(count($freq) > 200);
}

test_valid_sizes();
test_invalid_sizes();
test_uniqueness();
test_byte_distribution();
test_repeated_uniqueness();
test_binary_randomness();
