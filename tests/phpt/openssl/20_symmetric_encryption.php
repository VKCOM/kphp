@ok
<?php

const CBC_CIPHERS = ['aes-128-cbc', 'aes-256-cbc'];
const GCM_CIPHERS = ['aes-128-gcm', 'aes-256-gcm'];
const ALL_CIPHERS = ['aes-128-cbc', 'aes-256-cbc', 'aes-128-gcm', 'aes-256-gcm'];

function hx($r) {
    return $r === false ? 'false' : bin2hex($r);
}

function key_length(string $cipher): int {
    return strpos($cipher, '256') !== false ? 32 : 16;
}

function iv_length(string $cipher): int {
    return strpos($cipher, 'gcm') !== false ? 12 : 16;
}

function make_bytes(int $n, int $seed = 3): string {
    $s = '';
    for ($i = 0; $i < $n; ++$i) {
        $s .= chr(($i * 7 + $seed) & 255);
    }

    return $s;
}

function flip_byte($s, int $pos, int $mask = 1): string {
    $s = (string)$s;
    return substr($s, 0, $pos) . chr(ord($s[$pos]) ^ $mask) . substr($s, $pos + 1);
}

function xor_hex($a, $b): string {
    $a = (string)$a;
    $b = (string)$b;
    $r = '';
    for ($i = 0; $i < strlen($a); ++$i) {
        $r .= chr(ord($a[$i]) ^ ord($b[$i]));
    }
    
    return bin2hex($r);
}

function encrypt_any($data, string $cipher, $key, int $options, $iv, string &$tag, $aad = '', int $tag_length = 16) {
    $out_tag = '';
    $result = openssl_encrypt($data, $cipher, $key, $options, $iv, $out_tag, $aad, $tag_length);
    // PHP leaves the tag NULL for non-AEAD ciphers
    $tag = (string)$out_tag;
    return $result;
}

function test_known_answers() {
    $plain = hex2bin('6bc1bee22e409f96e93d7e117393172a');
    $iv = hex2bin('000102030405060708090a0b0c0d0e0f');

    $key128 = hex2bin('2b7e151628aed2a6abf7158809cf4f3c');
    $enc = openssl_encrypt($plain, 'aes-128-cbc', $key128, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv);
    var_dump(hx($enc));
    var_dump(openssl_decrypt($enc, 'aes-128-cbc', $key128, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv) === $plain);
    var_dump(openssl_encrypt($plain, 'aes-128-cbc', $key128, OPENSSL_ZERO_PADDING, $iv) === base64_encode($enc));

    $key256 = hex2bin('603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4');
    $enc = openssl_encrypt($plain, 'aes-256-cbc', $key256, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv);
    var_dump(hx($enc));
    var_dump(openssl_decrypt($enc, 'aes-256-cbc', $key256, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv) === $plain);

    $gcm_plain = hex2bin('d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a72'
        . '1c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39');
    $aad = hex2bin('feedfacedeadbeef' . 'feedfacedeadbeef' . 'abaddad2');
    $gcm_iv = hex2bin('cafebabefacedbaddecaf888');
    $gcm_key128 = hex2bin('feffe9928665731c6d6a8f9467308308');
    $gcm_key256 = hex2bin('feffe9928665731c6d6a8f9467308308' . 'feffe9928665731c6d6a8f9467308308');

    $tag = '';
    $enc = openssl_encrypt($gcm_plain, 'aes-128-gcm', $gcm_key128, OPENSSL_RAW_DATA, $gcm_iv, $tag, $aad);
    var_dump(hx($enc));
    var_dump(bin2hex($tag));
    var_dump(openssl_decrypt($enc, 'aes-128-gcm', $gcm_key128, OPENSSL_RAW_DATA, $gcm_iv, $tag, $aad) === $gcm_plain);

    $tag = '';
    $enc = openssl_encrypt($gcm_plain, 'aes-256-gcm', $gcm_key256, OPENSSL_RAW_DATA, $gcm_iv, $tag, $aad);
    var_dump(hx($enc));
    var_dump(bin2hex($tag));
    var_dump(openssl_decrypt($enc, 'aes-256-gcm', $gcm_key256, OPENSSL_RAW_DATA, $gcm_iv, $tag, $aad) === $gcm_plain);
}

function test_roundtrip_matrix() {
    $lengths = [0, 1, 2, 15, 16, 17, 31, 32, 33, 47, 48, 49, 255, 256, 257, 1000, 4096];

    foreach (ALL_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher), 11);
        $iv = make_bytes(iv_length($cipher), 5);

        foreach ($lengths as $len) {
            $data = make_bytes($len, 9);
            $tag = '';

            $raw = encrypt_any($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag);
            var_dump(strlen($raw));
            var_dump(md5($raw));
            var_dump(openssl_decrypt($raw, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag) === $data);

            $b64 = encrypt_any($data, $cipher, $key, 0, $iv, $tag);
            var_dump($b64 === base64_encode($raw));
            var_dump(openssl_decrypt($b64, $cipher, $key, 0, $iv, $tag) === $data);
        }
    }
}

function test_cbc_padding() {
    foreach (CBC_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher));
        $iv = make_bytes(16, 2);

        // PKCS#7 always adds a full block when the data is aligned
        foreach ([0, 1, 15, 16, 17, 32] as $len) {
            $enc = openssl_encrypt(str_repeat('a', $len), $cipher, $key, OPENSSL_RAW_DATA, $iv);
            var_dump(strlen($enc));
        }

        // without padding the data has to be aligned to the block size
        var_dump(hx(openssl_encrypt('', $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv)));
        var_dump(openssl_encrypt('abc', $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv));
        var_dump(openssl_encrypt(str_repeat('a', 15), $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv));
        var_dump(hx(openssl_encrypt(str_repeat('a', 16), $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv)));
        var_dump(openssl_encrypt(str_repeat('a', 17), $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv));
        var_dump(hx(openssl_encrypt(str_repeat('a', 48), $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv)));

        // the padding option is honoured with base64 output as well
        var_dump(openssl_encrypt('abc', $cipher, $key, OPENSSL_ZERO_PADDING, $iv));
        var_dump(openssl_encrypt(str_repeat('a', 16), $cipher, $key, OPENSSL_ZERO_PADDING, $iv));

        // decrypting padded data without padding exposes the padding bytes
        $padded = openssl_encrypt('hello', $cipher, $key, OPENSSL_RAW_DATA, $iv);
        var_dump(hx(openssl_decrypt($padded, $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv)));

        // data that is not a multiple of the block size can not be decrypted
        var_dump(openssl_decrypt('abc', $cipher, $key, OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_decrypt('abc', $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv));
        var_dump(openssl_decrypt($padded . 'x', $cipher, $key, OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_decrypt(substr($padded, 0, 15), $cipher, $key, OPENSSL_RAW_DATA, $iv));

        // padding that is not valid
        var_dump(openssl_decrypt(str_repeat("\0", 16), $cipher, $key, OPENSSL_RAW_DATA, $iv));
        var_dump(hx(openssl_decrypt(str_repeat("\0", 16), $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv)));
    }
}

function test_raw_data_option() {
    foreach (ALL_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher));
        $iv = make_bytes(iv_length($cipher));
        $tag = '';

        $raw = encrypt_any('some data', $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag);
        $b64 = encrypt_any('some data', $cipher, $key, 0, $iv, $tag);

        var_dump(hx($raw));
        var_dump($b64);
        var_dump($b64 === base64_encode($raw));

        var_dump(openssl_decrypt($raw, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag));
        var_dump(openssl_decrypt($b64, $cipher, $key, 0, $iv, $tag));

        // base64 is not accepted with the raw flag and raw bytes are not accepted without it
        var_dump(openssl_decrypt($b64, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag));
        var_dump(openssl_decrypt($raw, $cipher, $key, 0, $iv, $tag));
    }
}

function test_invalid_base64() {
    $key = make_bytes(16);
    $iv = make_bytes(16);
    $b64 = openssl_encrypt('hello world', 'aes-128-cbc', $key, 0, $iv);
    var_dump($b64);
    var_dump(openssl_decrypt($b64, 'aes-128-cbc', $key, 0, $iv));

    var_dump(openssl_decrypt('!!!', 'aes-128-cbc', $key, 0, $iv));
    var_dump(openssl_decrypt('A', 'aes-128-cbc', $key, 0, $iv));
    var_dump(openssl_decrypt('', 'aes-128-cbc', $key, 0, $iv));
    var_dump(openssl_decrypt('', 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv));
}

function test_empty_data() {
    $key = make_bytes(16);
    $iv = make_bytes(16);

    var_dump(hx(openssl_encrypt('', 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv)));
    var_dump(openssl_encrypt('', 'aes-128-cbc', $key, 0, $iv));
    var_dump(openssl_decrypt(openssl_encrypt('', 'aes-128-cbc', $key, 0, $iv), 'aes-128-cbc', $key, 0, $iv));
    var_dump(openssl_decrypt(openssl_encrypt('', 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv), 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv));

    var_dump(hx(openssl_decrypt('', 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv)));
    var_dump(hx(openssl_decrypt('', 'aes-128-cbc', $key, 0, $iv)));

    $gcm_iv = make_bytes(12);
    $tag = '';
    $enc = openssl_encrypt('', 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag);
    var_dump(hx($enc));
    var_dump(bin2hex($tag));
    var_dump(hx(openssl_decrypt('', 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag)));

    $enc = openssl_encrypt('', 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag, 'only aad');
    var_dump(hx($enc));
    var_dump(bin2hex($tag));
    var_dump(hx(openssl_decrypt('', 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag, 'only aad')));
    var_dump(hx(openssl_decrypt('', 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag, 'other aad')));
    var_dump(hx(openssl_decrypt('', 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag)));
}

function test_binary_and_large_data() {
    $all_bytes = '';
    for ($i = 0; $i < 256; ++$i) {
        $all_bytes .= chr($i);
    }
    $nul_data = "\0\0\0abc\0\0" . "\0";
    $large = make_bytes(200000, 17);

    foreach (ALL_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher), 1);
        $iv = make_bytes(iv_length($cipher), 2);
        $tag = '';

        foreach ([$all_bytes, $nul_data, $large] as $data) {
            $enc = encrypt_any($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag);
            var_dump(strlen($enc));
            var_dump(md5($enc));
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag) === $data);

            $enc = encrypt_any($data, $cipher, $key, 0, $iv, $tag);
            var_dump(strlen($enc));
            var_dump(md5($enc));
            var_dump(openssl_decrypt($enc, $cipher, $key, 0, $iv, $tag) === $data);
        }
    }
}

function test_key_lengths() {
    foreach (ALL_CIPHERS as $cipher) {
        $len = key_length($cipher);
        $iv = make_bytes(iv_length($cipher), 4);
        $tag = '';
        $exact = make_bytes($len, 21);

        $reference = encrypt_any('key length test', $cipher, $exact, OPENSSL_RAW_DATA, $iv, $tag);
        var_dump(hx($reference));

        // a short key is padded with zeros
        foreach ([1, 5, $len - 1] as $short_len) {
            $short = substr($exact, 0, $short_len);
            $padded = $short . str_repeat("\0", $len - $short_len);
            $enc = encrypt_any('key length test', $cipher, $short, OPENSSL_RAW_DATA, $iv, $tag);
            var_dump(hx($enc));
            var_dump($enc === encrypt_any('key length test', $cipher, $padded, OPENSSL_RAW_DATA, $iv, $tag));
            var_dump($enc === $reference);
            var_dump(openssl_decrypt($enc, $cipher, $padded, OPENSSL_RAW_DATA, $iv, $tag));
            var_dump(openssl_decrypt($enc, $cipher, $short, OPENSSL_RAW_DATA, $iv, $tag));
        }

        // an empty key is a key of zeros
        $enc = encrypt_any('key length test', $cipher, '', OPENSSL_RAW_DATA, $iv, $tag);
        var_dump(hx($enc));
        var_dump($enc === encrypt_any('key length test', $cipher, str_repeat("\0", $len), OPENSSL_RAW_DATA, $iv, $tag));
        var_dump(openssl_decrypt($enc, $cipher, str_repeat("\0", $len), OPENSSL_RAW_DATA, $iv, $tag));

        // a long key is truncated
        foreach ([$len + 1, $len + 16, 64, 100, 1000] as $long_len) {
            $long = $exact . make_bytes($long_len - $len, 99);
            $enc = encrypt_any('key length test', $cipher, $long, OPENSSL_RAW_DATA, $iv, $tag);
            var_dump(hx($enc));
            var_dump($enc === $reference);
            var_dump(openssl_decrypt($enc, $cipher, $exact, OPENSSL_RAW_DATA, $iv, $tag));
            var_dump(openssl_decrypt($reference, $cipher, $long, OPENSSL_RAW_DATA, $iv, $tag));
        }

        // a key with a different tail beyond the key length gives the same result
        var_dump(encrypt_any('key length test', $cipher, $exact . 'aaa', OPENSSL_RAW_DATA, $iv, $tag) === encrypt_any('key length test', $cipher, $exact . 'bbb', OPENSSL_RAW_DATA, $iv, $tag));

        // a key that differs inside the key length gives a different result
        $other = flip_byte($exact, $len - 1);
        var_dump(encrypt_any('key length test', $cipher, $other, OPENSSL_RAW_DATA, $iv, $tag) !== $reference);
    }

    // 128-bit key is not interchangeable with a 256-bit one
    $iv = make_bytes(16);
    $key16 = make_bytes(16, 21);
    var_dump(openssl_encrypt('data', 'aes-128-cbc', $key16, OPENSSL_RAW_DATA, $iv) !== openssl_encrypt('data', 'aes-256-cbc', $key16, OPENSSL_RAW_DATA, $iv));
    var_dump(hx(openssl_encrypt('data', 'aes-256-cbc', $key16, OPENSSL_RAW_DATA, $iv)));
    var_dump(openssl_encrypt('data', 'aes-256-cbc', $key16, OPENSSL_RAW_DATA, $iv) === openssl_encrypt('data', 'aes-256-cbc', $key16 . str_repeat("\0", 16), OPENSSL_RAW_DATA, $iv));
}

function test_dont_zero_pad_key() {
    foreach (ALL_CIPHERS as $cipher) {
        $len = key_length($cipher);
        $iv = make_bytes(iv_length($cipher), 4);
        $tag = '';
        $exact = make_bytes($len, 21);
        $options = OPENSSL_RAW_DATA | OPENSSL_DONT_ZERO_PAD_KEY;

        // exact key length behaves as usual
        $enc = encrypt_any('dont zero pad', $cipher, $exact, $options, $iv, $tag);
        var_dump(hx($enc));
        var_dump($enc === encrypt_any('dont zero pad', $cipher, $exact, OPENSSL_RAW_DATA, $iv, $tag));
        var_dump(openssl_decrypt($enc, $cipher, $exact, $options, $iv, $tag));

        // a short key is rejected instead of being padded
        foreach ([0, 1, $len - 1] as $short_len) {
            $short = substr($exact, 0, $short_len);
            $enc = encrypt_any('dont zero pad', $cipher, $short, $options, $iv, $tag);
            var_dump($enc);
            var_dump(openssl_decrypt($enc === false ? 'x' : $enc, $cipher, $short, $options, $iv, $tag));
            var_dump(openssl_decrypt(str_repeat('a', 16), $cipher, $short, $options, $iv, $tag));
        }

        // a long key is still truncated
        $long = $exact . make_bytes(20, 7);
        $enc = encrypt_any('dont zero pad', $cipher, $long, $options, $iv, $tag);
        var_dump(hx($enc));
        var_dump($enc === encrypt_any('dont zero pad', $cipher, $exact, OPENSSL_RAW_DATA, $iv, $tag));
    }

    // with base64 output
    $key = make_bytes(16);
    $iv = make_bytes(16);
    var_dump(openssl_encrypt('data', 'aes-128-cbc', $key, OPENSSL_DONT_ZERO_PAD_KEY, $iv));
    var_dump(openssl_encrypt('data', 'aes-128-cbc', 'short', OPENSSL_DONT_ZERO_PAD_KEY, $iv));
    var_dump(openssl_decrypt(openssl_encrypt('data', 'aes-128-cbc', $key, OPENSSL_DONT_ZERO_PAD_KEY, $iv), 'aes-128-cbc', $key, OPENSSL_DONT_ZERO_PAD_KEY, $iv));
}

function test_options_combinations() {
    $key = make_bytes(16);
    $iv = make_bytes(16);
    $block = str_repeat('b', 16);

    // all the combinations on aligned data
    for ($options = 0; $options < 8; ++$options) {
        $enc = openssl_encrypt($block, 'aes-128-cbc', $key, $options, $iv);
        var_dump($options);
        var_dump(hx($enc));
    }

    // unknown option bits are ignored
    var_dump(openssl_encrypt($block, 'aes-128-cbc', $key, 8 | OPENSSL_RAW_DATA, $iv) === openssl_encrypt($block, 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv));
    var_dump(openssl_encrypt($block, 'aes-128-cbc', $key, 16, $iv) === openssl_encrypt($block, 'aes-128-cbc', $key, 0, $iv));
    var_dump(openssl_encrypt($block, 'aes-128-cbc', $key, 1024 | OPENSSL_ZERO_PADDING, $iv) === openssl_encrypt($block, 'aes-128-cbc', $key, OPENSSL_ZERO_PADDING, $iv));

    // the flags are plain integers
    var_dump(OPENSSL_RAW_DATA);
    var_dump(OPENSSL_ZERO_PADDING);
    var_dump(OPENSSL_DONT_ZERO_PAD_KEY);

    // the default options value is 0
    var_dump(openssl_encrypt($block, 'aes-128-cbc', $key, 0, $iv) === openssl_encrypt($block, 'aes-128-cbc', $key, 0, $iv));
    var_dump(openssl_decrypt(openssl_encrypt($block, 'aes-128-cbc', $key, 0, $iv), 'aes-128-cbc', $key, 0, $iv) === $block);
}

function test_cbc_iv() {
    foreach (CBC_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher), 8);
        $exact = make_bytes(16, 33);
        $reference = openssl_encrypt('iv test data', $cipher, $key, OPENSSL_RAW_DATA, $exact);
        var_dump(hx($reference));

        // a short IV is padded with zeros
        foreach ([1, 8, 15] as $short_len) {
            $short = substr($exact, 0, $short_len);
            $padded = $short . str_repeat("\0", 16 - $short_len);
            $enc = openssl_encrypt('iv test data', $cipher, $key, OPENSSL_RAW_DATA, $short);
            var_dump(hx($enc));
            var_dump($enc === openssl_encrypt('iv test data', $cipher, $key, OPENSSL_RAW_DATA, $padded));
            var_dump($enc === $reference);
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $short));
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $padded));
        }

        // an empty IV is an IV of zeros
        $enc = openssl_encrypt('iv test data', $cipher, $key, OPENSSL_RAW_DATA, '');
        var_dump(hx($enc));
        var_dump($enc === openssl_encrypt('iv test data', $cipher, $key, OPENSSL_RAW_DATA, str_repeat("\0", 16)));
        var_dump($enc === openssl_encrypt('iv test data', $cipher, $key, OPENSSL_RAW_DATA));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, ''));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, str_repeat("\0", 16)));

        // a long IV is truncated
        foreach ([17, 20, 32, 100] as $long_len) {
            $long = $exact . make_bytes($long_len - 16, 77);
            $enc = openssl_encrypt('iv test data', $cipher, $key, OPENSSL_RAW_DATA, $long);
            var_dump(hx($enc));
            var_dump($enc === $reference);
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $exact));
            var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $long));
        }

        // a different IV only affects the first block when decrypting
        $data = str_repeat('0123456789abcdef', 4);
        $enc = openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $exact);
        $wrong_iv = flip_byte($exact, 0, 0x80);
        $dec = openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $wrong_iv);
        var_dump($dec !== false);
        var_dump($dec === $data);
        var_dump(strlen($dec));
        var_dump(substr($dec, 16) === substr($data, 16));
        var_dump(xor_hex(substr($dec, 0, 16), substr($data, 0, 16)));
    }
}

function test_gcm_iv() {
    foreach (GCM_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher), 8);

        // any non-empty IV length is accepted by GCM
        foreach ([1, 2, 8, 11, 12, 13, 16, 17, 32, 100] as $iv_len) {
            $iv = make_bytes($iv_len, 33);
            $tag = '';
            $enc = openssl_encrypt('gcm iv test', $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag);
            var_dump($iv_len);
            var_dump(hx($enc));
            var_dump(bin2hex($tag));
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag));

            // the IV is neither padded nor truncated
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv . "\0", $tag));
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, substr($iv, 0, -1), $tag));
        }

        // but not an empty one
        $tag = '';
        var_dump(openssl_encrypt('gcm iv test', $cipher, $key, OPENSSL_RAW_DATA, '', $tag));
        var_dump(openssl_decrypt(str_repeat('a', 11), $cipher, $key, OPENSSL_RAW_DATA, '', str_repeat('t', 16)));
    }
}

function test_gcm_tags() {
    foreach (GCM_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher), 6);
        $iv = make_bytes(12, 1);
        $data = 'gcm tag test data';
        $tag = '';

        $reference = openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, '', 16);
        $full_tag = $tag;
        var_dump(hx($reference));
        var_dump(bin2hex($full_tag));
        var_dump(strlen($full_tag));

        // the tag length can be shortened and the ciphertext does not change
        foreach ([1, 2, 4, 8, 12, 13, 14, 15, 16] as $tag_length) {
            $tag = '';
            $enc = openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, '', $tag_length);
            var_dump($tag_length);
            var_dump($enc === $reference);
            var_dump(strlen($tag));
            var_dump($tag === substr($full_tag, 0, $tag_length));
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag));
        }

        // the default tag length is 16
        $tag = '';
        openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag);
        var_dump(strlen($tag));

        // invalid tag lengths
        foreach ([0, 17, 18, 32, 100] as $tag_length) {
            $tag = '';
            var_dump(openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, '', $tag_length));
        }

        // AEAD encryption needs the tag to be requested
        var_dump(openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv));

        // truncated tags are verified as far as they go
        foreach ([4, 8, 12, 16] as $prefix_length) {
            var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv, substr($full_tag, 0, $prefix_length)));
        }

        // missing and oversized tags
        var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv, ''));
        var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv, $full_tag . 'x'));
        var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv, $full_tag . $full_tag));

        // wrong tags
        for ($i = 0; $i < 16; $i += 5) {
            $bad_tag = flip_byte($full_tag, $i);
            var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv, $bad_tag));
        }
        var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv, str_repeat("\0", 16)));
        var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv, strrev($full_tag)));

        // a wrong tag fails with base64 data as well
        $b64 = base64_encode($reference);
        var_dump(openssl_decrypt($b64, $cipher, $key, 0, $iv, $full_tag));
        var_dump(openssl_decrypt($b64, $cipher, $key, 0, $iv, str_repeat("\0", 16)));
    }
}

function test_gcm_tampering() {
    foreach (GCM_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher), 6);
        $iv = make_bytes(12, 1);
        $data = str_repeat('tamper me please ', 3);
        $aad = 'header';
        $tag = '';

        $enc = openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad);
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad) === $data);

        // flipped bits anywhere in the ciphertext
        foreach ([0, 1, 16, strlen($enc) - 1] as $pos) {
            $bad = flip_byte($enc, $pos);
            var_dump(openssl_decrypt($bad, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad));
        }

        // truncated, extended and swapped ciphertext
        var_dump(openssl_decrypt(substr($enc, 1), $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad));
        var_dump(openssl_decrypt(substr($enc, 0, -1), $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad));
        var_dump(openssl_decrypt($enc . 'x', $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad));
        var_dump(openssl_decrypt(strrev($enc), $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad));

        // wrong key, iv, aad
        $bad_key = flip_byte($key, 0);
        var_dump(openssl_decrypt($enc, $cipher, $bad_key, OPENSSL_RAW_DATA, $iv, $tag, $aad));
        $bad_iv = flip_byte($iv, 0);
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $bad_iv, $tag, $aad));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, 'Header'));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, ''));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad . 'x'));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, substr($aad, 1)));

        // short keys are not accepted as long as the zero padded key does not match
        var_dump(openssl_decrypt($enc, $cipher, substr($key, 0, 8), OPENSSL_RAW_DATA, $iv, $tag, $aad));
    }
}

function test_gcm_aad() {
    foreach (GCM_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher), 6);
        $iv = make_bytes(12, 1);
        $data = 'aad test data';

        $tag_none = '';
        $enc_none = openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag_none);
        $tag_empty = '';
        $enc_empty = openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag_empty, '');
        var_dump($enc_none === $enc_empty);
        var_dump($tag_none === $tag_empty);

        $aads = ['a', 'header', str_repeat('x', 15), str_repeat('x', 16), str_repeat('x', 17), make_bytes(300, 5), "\0", "a\0b", "\0\0\0\0", make_bytes(70000, 4)];
        foreach ($aads as $aad) {
            $tag = '';
            $enc = openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad);
            // the AAD does not change the ciphertext, only the tag
            var_dump($enc === $enc_none);
            var_dump($tag !== $tag_none);
            var_dump(md5($tag));
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, $aad));
            var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, ''));
        }

        // the AAD is bound to the exact bytes
        $tag1 = '';
        $tag2 = '';
        openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag1, "a\0");
        openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag2, "a");
        var_dump($tag1 !== $tag2);
    }
}

function test_aad_ignored_for_cbc() {
    foreach (CBC_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher));
        $iv = make_bytes(16);
        $tag = '';

        $reference = openssl_encrypt('cbc aad', $cipher, $key, OPENSSL_RAW_DATA, $iv);
        var_dump(openssl_encrypt('cbc aad', $cipher, $key, OPENSSL_RAW_DATA, $iv, $tag, 'ignored') === $reference);
        var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv, '', 'ignored'));
        var_dump(openssl_decrypt($reference, $cipher, $key, OPENSSL_RAW_DATA, $iv, '', ''));
    }
}

function test_tag_ignored_for_cbc() {
    foreach (CBC_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher));
        $iv = make_bytes(16);

        $enc = openssl_encrypt('cbc tag', $cipher, $key, OPENSSL_RAW_DATA, $iv);
        // a tag makes no sense for a block cipher, it is ignored
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, 'some tag'));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, str_repeat('t', 16)));
        var_dump(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA, $iv, "\0"));
    }
}

function test_wrong_key_and_iv() {
    foreach (CBC_CIPHERS as $cipher) {
        $key = make_bytes(key_length($cipher), 3);
        $iv = make_bytes(16, 3);
        $data = 'wrong key and wrong iv test';
        $enc = openssl_encrypt($data, $cipher, $key, OPENSSL_RAW_DATA, $iv);

        $bad_key = flip_byte($key, 0);
        var_dump(openssl_decrypt($enc, $cipher, $bad_key, OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_decrypt($enc, $cipher, str_repeat("\0", key_length($cipher)), OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_decrypt($enc, $cipher, '', OPENSSL_RAW_DATA, $iv));

        // without padding the garbage is visible
        var_dump(hx(openssl_decrypt($enc, $cipher, $bad_key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv)));
        var_dump(hx(openssl_decrypt($enc, $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv)));

        // wrong cipher for the data
        $other = $cipher === 'aes-128-cbc' ? 'aes-256-cbc' : 'aes-128-cbc';
        var_dump(hx(openssl_decrypt($enc, $other, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv)));

        // tampered ciphertext
        $bad = flip_byte($enc, strlen($enc) - 1);
        var_dump(openssl_decrypt($bad, $cipher, $key, OPENSSL_RAW_DATA, $iv));
        $bad = flip_byte($enc, 0);
        var_dump(hx(openssl_decrypt($bad, $cipher, $key, OPENSSL_RAW_DATA | OPENSSL_ZERO_PADDING, $iv)));
    }
}

function test_cipher_isolation() {
    $key = make_bytes(16);
    $iv = make_bytes(16);
    $gcm_iv = make_bytes(12);

    $cbc = openssl_encrypt('isolation', 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv);
    $tag = '';
    $gcm = openssl_encrypt('isolation', 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag);
    var_dump(hx($cbc));
    var_dump(hx($gcm));

    // a failed call does not poison the following ones
    var_dump(openssl_decrypt('garbage garbage!', 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv));
    var_dump(openssl_decrypt($cbc, 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv));
    var_dump(openssl_decrypt($gcm, 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, str_repeat('x', 16)));
    var_dump(openssl_decrypt($gcm, 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag));

    // the same call gives the same result every time
    for ($i = 0; $i < 20; ++$i) {
        $tag2 = '';
        var_dump(openssl_encrypt('isolation', 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv) === $cbc);
        var_dump(openssl_encrypt('isolation', 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag2) === $gcm);
        var_dump($tag2 === $tag);
    }
}

function test_unknown_algorithms() {
    $key = make_bytes(16);
    $iv = make_bytes(16);
    $tag = '';

    $unknown = ['', 'unknown', 'aes', 'aes-128', 'aes-128-', 'aes-129-cbc', 'aes-128-cbcc', 'aes-128-cb', 'aes_128_cbc', 'md5', 'sha256', 'rsa', ' ', "\0", 'aes-128-gcm-', '-aes-128-cbc'];
    foreach ($unknown as $method) {
        var_dump(openssl_encrypt('data', $method, $key, OPENSSL_RAW_DATA, $iv, $tag));
        var_dump(openssl_decrypt('data', $method, $key, OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_decrypt('data', $method, $key, 0, $iv));
        var_dump(openssl_cipher_iv_length($method));
    }

    // surrounding whitespace and embedded NULs are part of the name
    $bad_names = [' aes-128-cbc', 'aes-128-cbc ', "\naes-128-cbc", "aes-128-cbc\n", "\0aes-128-cbc", "aes\0-128-cbc", "aes-128\0-cbc", "\0\0aes-128-cbc"];
    foreach ($bad_names as $method) {
        var_dump(openssl_encrypt('data', $method, $key, OPENSSL_RAW_DATA, $iv, $tag));
        var_dump(openssl_decrypt('data', $method, $key, OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_cipher_iv_length($method));
    }
}

function test_algorithm_name_case() {
    $key = make_bytes(16);
    $iv = make_bytes(16);
    $gcm_iv = make_bytes(12);

    $reference = openssl_encrypt('case test', 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv);
    foreach (['AES-128-CBC', 'Aes-128-Cbc', 'aes-128-CBC', 'AES-128-cbc', 'aEs-128-cBc'] as $name) {
        var_dump(openssl_encrypt('case test', $name, $key, OPENSSL_RAW_DATA, $iv) === $reference);
        var_dump(openssl_decrypt($reference, $name, $key, OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_cipher_iv_length($name));
    }

    $key256 = make_bytes(32);
    $reference = openssl_encrypt('case test', 'aes-256-cbc', $key256, OPENSSL_RAW_DATA, $iv);
    foreach (['AES-256-CBC', 'Aes-256-Cbc'] as $name) {
        var_dump(openssl_encrypt('case test', $name, $key256, OPENSSL_RAW_DATA, $iv) === $reference);
        var_dump(openssl_decrypt($reference, $name, $key256, OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_cipher_iv_length($name));
    }

    $tag = '';
    $reference = openssl_encrypt('case test', 'aes-128-gcm', $key, OPENSSL_RAW_DATA, $gcm_iv, $tag);
    foreach (['AES-128-GCM', 'Aes-128-Gcm', 'aes-128-GCM'] as $name) {
        $tag2 = '';
        var_dump(openssl_encrypt('case test', $name, $key, OPENSSL_RAW_DATA, $gcm_iv, $tag2) === $reference);
        var_dump($tag2 === $tag);
        var_dump(openssl_decrypt($reference, $name, $key, OPENSSL_RAW_DATA, $gcm_iv, $tag));
        var_dump(openssl_cipher_iv_length($name));
    }

    $tag = '';
    $reference = openssl_encrypt('case test', 'aes-256-gcm', $key256, OPENSSL_RAW_DATA, $gcm_iv, $tag);
    foreach (['AES-256-GCM', 'Aes-256-Gcm'] as $name) {
        $tag2 = '';
        var_dump(openssl_encrypt('case test', $name, $key256, OPENSSL_RAW_DATA, $gcm_iv, $tag2) === $reference);
        var_dump($tag2 === $tag);
        var_dump(openssl_decrypt($reference, $name, $key256, OPENSSL_RAW_DATA, $gcm_iv, $tag));
        var_dump(openssl_cipher_iv_length($name));
    }
}

function test_algorithm_name_with_trailing_nul() {
    $key = make_bytes(16);
    $iv = make_bytes(16);

    // the name is cut at the first NUL byte by the C string handling in PHP
    $reference = openssl_encrypt('nul test', 'aes-128-cbc', $key, OPENSSL_RAW_DATA, $iv);
    foreach (["aes-128-cbc\0", "aes-128-cbc\0\0", "aes-128-cbc\0junk", "aes-128-cbc\0aes-256-cbc", "AES-128-CBC\0x"] as $name) {
        var_dump(openssl_encrypt('nul test', $name, $key, OPENSSL_RAW_DATA, $iv) === $reference);
        var_dump(openssl_decrypt($reference, $name, $key, OPENSSL_RAW_DATA, $iv));
        var_dump(openssl_cipher_iv_length($name));
    }
}

function test_cipher_iv_length() {
    var_dump(openssl_cipher_iv_length('aes-128-cbc'));
    var_dump(openssl_cipher_iv_length('aes-256-cbc'));
    var_dump(openssl_cipher_iv_length('aes-128-gcm'));
    var_dump(openssl_cipher_iv_length('aes-256-gcm'));

    foreach (ALL_CIPHERS as $cipher) {
        var_dump(openssl_cipher_iv_length($cipher) === iv_length($cipher));
        var_dump(is_int(openssl_cipher_iv_length($cipher)));
    }

    var_dump(openssl_cipher_iv_length('') === false);
    var_dump(openssl_cipher_iv_length('no-such-cipher') === false);
}

function test_cipher_methods_list() {
    $methods = openssl_get_cipher_methods();
    var_dump(is_array($methods));
    var_dump(count($methods) >= 4);

    foreach (ALL_CIPHERS as $cipher) {
        var_dump(in_array($cipher, $methods, true));
        var_dump(in_array($cipher, openssl_get_cipher_methods(true), true));
        var_dump(in_array($cipher, openssl_get_cipher_methods(false), true));
    }

    // every reported name that is among the supported ones is usable
    $usable = 0;
    foreach ($methods as $name) {
        if (in_array($name, ALL_CIPHERS, true)) {
            var_dump(openssl_cipher_iv_length($name) !== false);
            ++$usable;
        }
    }
    var_dump($usable);

    // no duplicates and only strings
    var_dump(count($methods) === count(array_unique($methods)));
    $all_strings = true;
    foreach ($methods as $name) {
        $all_strings = $all_strings && is_string($name) && $name !== '';
    }
    var_dump($all_strings);
}

function test_independent_calls_in_sequence() {
    $key = make_bytes(32, 40);
    $messages = ['first', 'second message', '', str_repeat('third', 100), "fourth\0binary\xff"];
    $encrypted = [];

    for ($i = 0; $i < count($messages); ++$i) {
        $iv = make_bytes(16, 50 + $i);
        $encrypted[] = [openssl_encrypt($messages[$i], 'aes-256-cbc', $key, 0, $iv), $iv];
    }

    for ($i = 0; $i < count($encrypted); ++$i) {
        var_dump($encrypted[$i][0]);
        var_dump(openssl_decrypt($encrypted[$i][0], 'aes-256-cbc', $key, 0, $encrypted[$i][1]) === $messages[$i]);
    }

    // the order of the decryption does not matter
    for ($i = count($encrypted) - 1; $i >= 0; --$i) {
        var_dump(openssl_decrypt($encrypted[$i][0], 'aes-256-cbc', $key, 0, $encrypted[$i][1]) === $messages[$i]);
    }
}

test_known_answers();
test_roundtrip_matrix();
test_cbc_padding();
test_raw_data_option();
test_invalid_base64();
test_empty_data();
test_binary_and_large_data();
test_key_lengths();
test_dont_zero_pad_key();
test_options_combinations();
test_cbc_iv();
test_gcm_iv();
test_gcm_tags();
test_gcm_tampering();
test_gcm_aad();
test_aad_ignored_for_cbc();
test_tag_ignored_for_cbc();
test_wrong_key_and_iv();
test_cipher_isolation();
test_unknown_algorithms();
test_algorithm_name_case();
test_algorithm_name_with_trailing_nul();
test_cipher_iv_length();
test_cipher_methods_list();
test_independent_calls_in_sequence();
