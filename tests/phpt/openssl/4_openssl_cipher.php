@ok k2_skip
<?php

/**
 * @param bool $aliases
 */
function test_openssl_get_cipher_methods($aliases) {
    $methods = openssl_get_cipher_methods($aliases);
    $kphp_methods = $methods;
#ifndef KPHP
    $kphp_methods = array_filter($methods, function ($method) {
        return stristr($method, "gcm") === false && stristr($method, "ccm") === false;
    });
#endif
    $kphp_methods = array_unique(array_map('strtolower', array_values($kphp_methods)));
    sort($kphp_methods);
    // when KPHP uses openssl 1.1.1 as PHP7 does we will uncomment the next line
    // echo implode("\n", $kphp_methods);
}

function test_basic_encrypt_decrypt() {
    $data = "openssl_encrypt() and openssl_decrypt() tests";
    $method = "AES-128-CBC";
    $password = "openssl";

    $ivlen = openssl_cipher_iv_length($method);
    $iv    = '';
    srand(time() + intval(microtime(true)));
    while(strlen($iv) < $ivlen) $iv .= chr(rand(0,255));

    $encrypted = openssl_encrypt($data, $method, $password, 0, $iv);
    $output = openssl_decrypt($encrypted, $method, $password, 0, $iv);
    var_dump($output);

    $encrypted = openssl_encrypt($data, $method, $password, OPENSSL_RAW_DATA, $iv);
    $output = openssl_decrypt($encrypted, $method, $password, OPENSSL_RAW_DATA, $iv);
    var_dump($output);

    // if we want to manage our own padding
    $padded_data = $data . str_repeat(' ', 16 - (strlen($data) % 16));
    $encrypted = openssl_encrypt($padded_data, $method, $password, OPENSSL_RAW_DATA|OPENSSL_ZERO_PADDING, $iv);
    $output = openssl_decrypt($encrypted, $method, $password, OPENSSL_RAW_DATA|OPENSSL_ZERO_PADDING, $iv);
    var_dump(rtrim($output));

    $tag = 'hello world';
    $encrypted = openssl_encrypt($data, $method, $password, 0, $iv, $tag);
    // in PHP it is NULL, in KPHP it is an empty string
    var_dump((bool)$tag);
    $tag = 'hello world';
    $output = openssl_decrypt($encrypted, $method, $password, 0, $iv, $tag);
    var_dump($output);
}

function test_basic_encrypt_decrypt_gcm_ccm() {
    $methods = ['aes-128-gcm'];
    foreach($methods as $method) {
        $key = '01234567890123456789012345678901';
        $data = 'test';
        $add = 'aad';
        $tag = '';

        $iv = openssl_random_pseudo_bytes(openssl_cipher_iv_length($method));
        $encrypted = openssl_encrypt($data, $method, $key, OPENSSL_RAW_DATA, $iv, $tag, $add, 16);

        $decrypted_data = openssl_decrypt($encrypted, $method, $key, OPENSSL_RAW_DATA, $iv, $tag, $add);
        var_dump($data === $decrypted_data);
    }
}

function test_decrypt_errors() {
    $data = "openssl_decrypt() tests";
    $method = "AES-128-CBC";
    $password = "openssl";
    $wrong = base64_encode("wrong");
    $iv = str_repeat("\0", openssl_cipher_iv_length($method));

    $encrypted = openssl_encrypt($data, $method, $password);
    var_dump($encrypted); /* Not passing $iv should be the same as all-NULL iv, but with a warning */
    var_dump(openssl_encrypt($data, $method, $password, 0, $iv));
    var_dump(openssl_decrypt($encrypted, $method, $wrong));
    var_dump(openssl_decrypt($encrypted, $wrong, $password));
    var_dump(openssl_decrypt($wrong, $method, $password));
    var_dump(openssl_decrypt($wrong, $wrong, $password));
    var_dump(openssl_decrypt($encrypted, $wrong, $wrong));
    var_dump(openssl_decrypt($wrong, $wrong, $wrong));
}

function test_encrypt_errors() {
    $data = "openssl_encrypt() tests";
    $method = "AES-128-CBC";
    $password = "openssl";
    $iv = str_repeat("\0", openssl_cipher_iv_length($method));
    $wrong = "wrong";

    // wrong parameters tests
    var_dump(openssl_encrypt($data, $wrong, $password));
}


test_openssl_get_cipher_methods(true);
test_openssl_get_cipher_methods(false);
test_basic_encrypt_decrypt();
test_basic_encrypt_decrypt_gcm_ccm();
test_decrypt_errors();
test_encrypt_errors();
