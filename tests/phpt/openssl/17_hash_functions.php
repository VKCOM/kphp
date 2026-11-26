@ok benchmark
<?php

// algorithms supported by both runtimes (K2 additionally supports ripemd160, md2 and md4)
const ALGOS = ['md5', 'sha1', 'sha224', 'sha256', 'sha384', 'sha512'];

const DIGEST_SIZES = [
    'md5' => 16,
    'sha1' => 20,
    'sha224' => 28,
    'sha256' => 32,
    'sha384' => 48,
    'sha512' => 64,
];

const BLOCK_SIZES = [
    'md5' => 64,
    'sha1' => 64,
    'sha224' => 64,
    'sha256' => 64,
    'sha384' => 128,
    'sha512' => 128,
];

function all_bytes(): string {
    $res = '';
    for ($i = 0; $i < 256; ++$i) {
        $res .= chr($i);
    }
    
    return $res;
}

function test_known_vectors() {
    var_dump(hash('md5', ''));
    var_dump(hash('md5', 'abc'));
    var_dump(hash('md5', 'The quick brown fox jumps over the lazy dog'));
    var_dump(hash('sha1', ''));
    var_dump(hash('sha1', 'abc'));
    var_dump(hash('sha1', 'The quick brown fox jumps over the lazy dog'));
    var_dump(hash('sha224', ''));
    var_dump(hash('sha224', 'abc'));
    var_dump(hash('sha256', ''));
    var_dump(hash('sha256', 'abc'));
    var_dump(hash('sha384', ''));
    var_dump(hash('sha384', 'abc'));
    var_dump(hash('sha512', ''));
    var_dump(hash('sha512', 'abc'));

    var_dump(md5(''));
    var_dump(md5('abc'));
    var_dump(sha1(''));
    var_dump(sha1('abc'));
}

function test_hash_rfc4231_hmac_vectors() {
    $key = str_repeat("\x0b", 20);
    foreach (['sha224', 'sha256', 'sha384', 'sha512'] as $algo) {
        var_dump(hash_hmac($algo, 'Hi There', $key));
    }

    foreach (['sha224', 'sha256', 'sha384', 'sha512'] as $algo) {
        var_dump(hash_hmac($algo, 'what do ya want for nothing?', 'Jefe'));
    }

    var_dump(hash_hmac('md5', 'Hi There', str_repeat("\x0b", 16)));
    var_dump(hash_hmac('md5', 'what do ya want for nothing?', 'Jefe'));
    var_dump(hash_hmac('sha1', 'Hi There', str_repeat("\x0b", 20)));
    var_dump(hash_hmac('sha1', 'what do ya want for nothing?', 'Jefe'));
}

function test_digest_sizes() {
    $data = 'size test';
    foreach (ALGOS as $algo) {
        var_dump(strlen(hash($algo, $data, true)) === DIGEST_SIZES[$algo]);
        var_dump(strlen(hash($algo, $data, false)) === 2 * DIGEST_SIZES[$algo]);
        var_dump(strlen(hash($algo, $data)) === 2 * DIGEST_SIZES[$algo]);
        var_dump(strlen(hash_hmac($algo, $data, 'key', true)) === DIGEST_SIZES[$algo]);
        var_dump(strlen(hash_hmac($algo, $data, 'key', false)) === 2 * DIGEST_SIZES[$algo]);
        var_dump(strlen(hash_hmac($algo, $data, 'key')) === 2 * DIGEST_SIZES[$algo]);
    }

    var_dump(strlen(md5($data, true)));
    var_dump(strlen(md5($data, false)));
    var_dump(strlen(md5($data)));
    var_dump(strlen(sha1($data, true)));
    var_dump(strlen(sha1($data, false)));
    var_dump(strlen(sha1($data)));
}

function test_raw_output_matches_hex() {
    $data = all_bytes();
    foreach (ALGOS as $algo) {
        var_dump(bin2hex(hash($algo, $data, true)) === hash($algo, $data));
        var_dump(bin2hex(hash_hmac($algo, $data, $data, true)) === hash_hmac($algo, $data, $data));
        var_dump(hash($algo, $data, false) === hash($algo, $data));
        var_dump(hash_hmac($algo, $data, 'k', false) === hash_hmac($algo, $data, 'k'));
    }
    var_dump(bin2hex(md5($data, true)) === md5($data));
    var_dump(bin2hex(sha1($data, true)) === sha1($data));
}

function test_raw_output_values() {
    var_dump(bin2hex(hash('md5', 'abc', true)));
    var_dump(bin2hex(hash('sha1', 'abc', true)));
    var_dump(bin2hex(hash('sha256', 'abc', true)));
    var_dump(bin2hex(hash_hmac('sha256', 'data', 'key', true)));
    var_dump(base64_encode(hash('sha256', 'abc', true)));
    var_dump(base64_encode(md5('abc', true)));
    var_dump(base64_encode(sha1('abc', true)));
}

function test_function_equivalence() {
    $values = ['', 'a', 'abc', "\0", "a\0b", all_bytes(), str_repeat('x', 1000)];
    foreach ($values as $v) {
        var_dump(md5($v) === hash('md5', $v));
        var_dump(sha1($v) === hash('sha1', $v));
        var_dump(md5($v, true) === hash('md5', $v, true));
        var_dump(sha1($v, true) === hash('sha1', $v, true));
    }
}

function test_hash_all_algos_values() {
    $values = ['', 'a', 'abc', "\0", "a\0b", "\xff\xfe\xfd", 'привет мир', "line1\nline2\r\n", all_bytes()];
    foreach (ALGOS as $algo) {
        foreach ($values as $v) {
            var_dump(hash($algo, $v));
            var_dump(bin2hex(hash($algo, $v, true)));
        }
    }
}

function test_hash_block_boundaries() {
    // lengths around the block size (64 / 128 bytes) and the padding boundaries (55/56, 111/112)
    $lengths = [0, 1, 54, 55, 56, 57, 63, 64, 65, 110, 111, 112, 113, 119, 120, 127, 128, 129, 191, 192, 255, 256, 257];
    foreach (ALGOS as $algo) {
        foreach ($lengths as $len) {
            var_dump(hash($algo, str_repeat('a', $len)));
        }
    }

    foreach ($lengths as $len) {
        var_dump(md5(str_repeat('b', $len)));
        var_dump(sha1(str_repeat('b', $len)));
    }
}

function test_hash_large_data() {
    $big = str_repeat('0123456789abcdef', 65536); // 1 MiB
    var_dump(strlen($big));
    foreach (ALGOS as $algo) {
        var_dump(hash($algo, $big));
        var_dump(hash_hmac($algo, $big, 'key'));
    }
    
    var_dump(md5($big));
    var_dump(sha1($big));
}

function test_hash_binary_and_nul() {
    var_dump(hash('sha256', "\0") !== hash('sha256', ''));
    var_dump(hash('sha256', "\0") !== hash('sha256', "\0\0"));
    var_dump(hash('sha256', "a\0b") !== hash('sha256', 'a'));
    var_dump(hash('sha256', "a\0b") !== hash('sha256', 'ab'));
    var_dump(hash('sha256', "a\0b"));
    var_dump(hash('sha256', "\0\0\0\0"));

    var_dump(md5("a\0b"));
    var_dump(sha1("a\0b"));
    var_dump(hash_hmac('sha256', "a\0b", "k\0ey"));
    var_dump(hash_hmac('sha256', "a\0b", "k\0ey") !== hash_hmac('sha256', "a\0b", "k"));
    // a key is zero padded to the block size, so "\0" is equivalent to an empty key
    var_dump(hash_hmac('sha256', "data", "\0") === hash_hmac('sha256', "data", ""));
}

function test_hash_non_string_args() {
    var_dump(hash('md5', 123));
    var_dump(hash('md5', '123') === hash('md5', 123));
    var_dump(hash('sha1', true));
    var_dump(hash('sha1', false));
    var_dump(hash('sha1', null));
    var_dump(md5(12345));
    var_dump(sha1(12345));
    var_dump(hash_hmac('sha256', 123, 456));
    var_dump(hash_hmac('sha256', '123', '456') === hash_hmac('sha256', 123, 456));
}

function test_hmac_keys() {
    $data = 'message';
    foreach (ALGOS as $algo) {
        $block = BLOCK_SIZES[$algo];

        // empty key
        var_dump(hash_hmac($algo, $data, ''));
        var_dump(hash_hmac($algo, '', ''));
        var_dump(hash_hmac($algo, '', 'key'));

        // keys around the block size
        foreach ([1, $block - 1, $block, $block + 1, 2 * $block, 2 * $block + 1, 1000] as $len) {
            var_dump(hash_hmac($algo, $data, str_repeat('k', $len)));
        }

        // a key longer than the block size is replaced with its digest
        $long_key = str_repeat('K', $block + 10);
        var_dump(hash_hmac($algo, $data, $long_key) === hash_hmac($algo, $data, hash($algo, $long_key, true)));

        // binary keys
        var_dump(hash_hmac($algo, $data, all_bytes()));
        var_dump(hash_hmac($algo, $data, "\0\0\0\0"));
        var_dump(hash_hmac($algo, $data, "\xff\xff\xff"));

        // zero padded key is equivalent to the unpadded one when it fits into the block
        var_dump(hash_hmac($algo, $data, 'key') === hash_hmac($algo, $data, 'key' . str_repeat("\0", $block - 3)));

        // different keys / data give different macs
        var_dump(hash_hmac($algo, $data, 'key1') !== hash_hmac($algo, $data, 'key2'));
        var_dump(hash_hmac($algo, 'data1', 'key') !== hash_hmac($algo, 'data2', 'key'));

        // hmac differs from the plain hash
        var_dump(hash_hmac($algo, $data, 'key') !== hash($algo, $data));
        var_dump(hash_hmac($algo, $data, 'key') !== hash($algo, 'key' . $data));
    }
}

function test_hmac_large_data() {
    $data = str_repeat("\xAB\xCD", 100000);
    foreach (ALGOS as $algo) {
        var_dump(hash_hmac($algo, $data, str_repeat('s', 200)));
        var_dump(hash_hmac($algo, $data, str_repeat('s', 200), true) === hex2bin(hash_hmac($algo, $data, str_repeat('s', 200))));
    }
}

function test_hash_equals() {
    var_dump(hash_equals('', ''));
    var_dump(hash_equals('a', 'a'));
    var_dump(hash_equals('a', 'b'));
    var_dump(hash_equals('abc', 'abc'));
    var_dump(hash_equals('abc', 'abd'));
    var_dump(hash_equals('abc', 'Abc'));
    var_dump(hash_equals('abc', 'ab'));
    var_dump(hash_equals('ab', 'abc'));
    var_dump(hash_equals('', 'a'));
    var_dump(hash_equals('a', ''));

    // binary and NUL bytes
    var_dump(hash_equals("\0", "\0"));
    var_dump(hash_equals("\0", ''));
    var_dump(hash_equals("a\0b", "a\0b"));
    var_dump(hash_equals("a\0b", "a\0c"));
    var_dump(hash_equals("a\0b", "a\0"));
    var_dump(hash_equals("\xff", "\xff"));
    var_dump(hash_equals("\xff", "\x7f"));
    var_dump(hash_equals(all_bytes(), all_bytes()));

    // difference only in the first / last / middle byte
    $s = str_repeat('x', 100);
    var_dump(hash_equals($s, $s));
    var_dump(hash_equals($s, 'y' . substr($s, 1)));
    var_dump(hash_equals($s, substr($s, 0, 99) . 'y'));
    var_dump(hash_equals($s, substr($s, 0, 50) . 'y' . substr($s, 51)));

    // difference in a single bit
    var_dump(hash_equals("\x00", "\x01"));
    var_dump(hash_equals("\x80", "\x00"));
    var_dump(hash_equals("\x80\x00", "\x00\x00"));

    // digests
    var_dump(hash_equals(hash('sha256', 'abc'), hash('sha256', 'abc')));
    var_dump(hash_equals(hash('sha256', 'abc'), hash('sha256', 'abd')));
    var_dump(hash_equals(md5('abc'), md5('abc')));
    var_dump(hash_equals(md5('abc'), sha1('abc')));
    var_dump(hash_equals(hash_hmac('sha256', 'm', 'k', true), hash_hmac('sha256', 'm', 'k', true)));
    var_dump(hash_equals(hash_hmac('sha256', 'm', 'k', true), hash_hmac('sha256', 'm', 'k2', true)));

    // large strings
    $big1 = str_repeat('0123456789', 100000);
    $big2 = str_repeat('0123456789', 100000);
    var_dump(hash_equals($big1, $big2));
    $big2 = substr($big2, 0, 999999) . 'x';
    var_dump(hash_equals($big1, $big2));
    var_dump(hash_equals($big1, $big2 . 'x'));
}

function test_crc32_values() {
    $values = ['', 'a', 'abc', '123456789', 'The quick brown fox jumps over the lazy dog', "\0", "\0\0\0\0", "a\0b", "\xff", "\xff\xff\xff\xff", 'привет мир', "line1\nline2\r\n", all_bytes()];
    foreach ($values as $v) {
        var_dump(crc32($v));
        var_dump(dechex(crc32($v)));
    }
}

function test_crc32_properties() {
    // result is always in [0, 2^32)
    $values = ['', 'a', 'abc', "\xff\xff\xff\xff", all_bytes(), str_repeat('z', 1000)];
    foreach ($values as $v) {
        $c = crc32($v);
        var_dump($c >= 0 && $c <= 0xFFFFFFFF);
    }

    // deterministic
    var_dump(crc32('abc') === crc32('abc'));
    var_dump(crc32('abc') !== crc32('abd'));
    var_dump(crc32("a\0b") !== crc32('ab'));
    var_dump(crc32('') === 0);

    // non-string arguments
    var_dump(crc32(123456789));
    var_dump(crc32(123456789) === crc32('123456789'));
    var_dump(crc32(true) === crc32('1'));
    var_dump(crc32(false) === crc32(''));
    var_dump(crc32(null) === crc32(''));
}

function test_crc32_lengths() {
    $lengths = [0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 257, 1023, 1024, 1025, 4095, 4096, 4097];
    foreach ($lengths as $len) {
        var_dump(crc32(str_repeat('a', $len)));
    }

    $all = all_bytes();
    foreach ([1, 7, 8, 9, 100, 255, 256] as $len) {
        var_dump(crc32(substr($all, 0, $len)));
    }
    
    var_dump(crc32(str_repeat("\x00", 100000)));
    var_dump(crc32(str_repeat("\xff", 100000)));
    var_dump(crc32(str_repeat('0123456789abcdef', 65536)));
}

function test_hash_algos() {
    $algos = hash_algos();
    $hmac_algos = hash_hmac_algos();

    foreach (ALGOS as $algo) {
        var_dump(in_array($algo, $algos, true));
        var_dump(in_array($algo, $hmac_algos, true));
    }

    // the lists contain unique, non-empty, lowercase names
    var_dump(count($algos) === count(array_unique($algos)));
    var_dump(count($hmac_algos) === count(array_unique($hmac_algos)));
    // the number of algorithms differs between PHP and KPHP, so print a single summary value
    $names_ok = true;
    foreach ($algos as $algo) {
        $names_ok = $names_ok && $algo !== '' && $algo === strtolower($algo);
    }

    var_dump($names_ok);

    $hmac_names_ok = true;
    foreach ($hmac_algos as $algo) {
        $hmac_names_ok = $hmac_names_ok && $algo !== '' && $algo === strtolower($algo);
    }

    var_dump($hmac_names_ok);

    // every algorithm from the lists is usable
    foreach ($algos as $algo) {
        if (!in_array($algo, ALGOS, true)) {
            continue;
        }
        var_dump(strlen(hash($algo, 'abc', true)) === DIGEST_SIZES[$algo]);
    }

    foreach ($hmac_algos as $algo) {
        if (!in_array($algo, ALGOS, true)) {
            continue;
        }
        var_dump(strlen(hash_hmac($algo, 'abc', 'key', true)) === DIGEST_SIZES[$algo]);
    }
}

test_known_vectors();
test_hash_rfc4231_hmac_vectors();
test_digest_sizes();
test_raw_output_matches_hex();
test_raw_output_values();
test_function_equivalence();
test_hash_all_algos_values();
test_hash_block_boundaries();
test_hash_large_data();
test_hash_binary_and_nul();
test_hash_non_string_args();
test_hmac_keys();
test_hmac_large_data();
test_hash_equals();
test_crc32_values();
test_crc32_properties();
test_crc32_lengths();
test_hash_algos();
