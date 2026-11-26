@ok
<?php

$private_key_pem = <<<EOF
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCILnpcP49RueO9
K+c8zXxcFg08RzTDUb08lByhg3NmcESJJeqeOE82RK7pXipBV/wIck5JKLmCwmR7
JLgdBEfvdCgoJGT6TTLI0yZgNkEoOs+RnE3zc5EaOyzPQAcZBXpnh90izrgBTyp6
za77iwnShL3/bBVbO5dNcXYxaIBFd3gkHJLHZ1rsREh1vBy/b207rZbtNL6jYzYB
FXvEm4iajF8uvZWItR9VQAssUNDDfhLIp+DPEDKJZqeqBH8nDsJZOl36Nag5vRPF
KVwPztthlNc+/Ee3hThf5raWb8fIVSs7EkQksuZynOzpoSwnHWmnvtL7bZmvaeV4
fJDg8QQJAgMBAAECggEAEsbjJOBiq4XHtVKzYeUstinC420nWkhlS6VZw+9dtGHb
IVSdYqQoT+GX2HedDet1cINfm04b3hVdnhkfBb6MXfjp0eHKxom/Fy+or/AV0IKO
Ej63WiUKJyVs9UVCdUcVweawLVd2yZTCf7I9OVPKI+vHostNWon/P7SwSPm8DgJ1
NTJLmjbt1as5FpbV+7h/726dsBHmqlQ35YFM1Kyk3XNumeNNvb+mJDcAx7v8+6Mx
bkntDJFSesxzYu6E+OwH7rJKLKYT2CWCfZASgZQlMpEtQgcz0Jl7Nmb11fu7QIwE
r74wmshQQm7PPY9qaBPkoUYsyqkbX0FSxfI/xtyr3QKBgQC+CC9knnUc3P3+MlHw
C6Y4t4jlzaQY6C9nUhRjZ266BbNLkh+rb94BllHVgjggJ9hHN0y4LWZHggjB6J7S
pDQ1co0pXgUr0P2pi5MpzAQKHZ5LEesOl+uKKVBG+p0Olmip66lNBPpPzHWKhguK
o/jGFNzBO0BZwF/Sc1Vjw/VV4wKBgQC3dLLdI++H4V5n5uQQDePm3YmU0i3NQ5sE
Uf224YKfUv2yTGEcFrRZTxa2W0UU6DWbLQKi9MrkmCxXysSEKBfvtIkWnD0p8+7L
CUSakTDBYWWY0wfcGj86WLYG1hKVCbc/VhIzCIo4a8N7nHZO6BzKVYG8W+tkDcRo
6zFOrmOCIwKBgBAKaNW5enFBRj9ayAfXM+S9uVSMkzl2/+lcyFPLu1EAgi3y27GA
8+B2a9n17/7m3h7E0DkLvAJ1O4ACvsVRDwcj8D55rT4RmhfhrD5u9jhUX7Q+gvTs
Zfl3m0sihHdiwtr1BYl2zhpGS/Wxcu/0/17ZC/lTow7waiY9jgup0t+5AoGBAIV/
TDET11gfrZ0aLVdpZDA5gcZj86Rsz78jEucjzvO1Zpt6bSgNdYuPu81TOT/vgiK7
bDG8xG/4NDxmdv5cVp691ww3AWMn+NqSkfOQN9Rg1ZeR6SRHjT8eEseCu97sRQa3
0WKFkl9gO84VmxnsWhb6hf/VO8OBKS3YB/dje1HVAoGAXbcd3Frvrn941UN/f9dL
uAd7Pc/RRJ2A948xj7hlQJHVHs3Lu8X4misLPOMVImCtJ2lo10/wOyHIxwzGNjdX
VYEXd5FALCMrfMMt0ScORM+RD5JB+vVZsggxav6oAl9VknmCP8CP+ovvCtSv5mAk
HXwDmuSC1cCdGCsm2PMBrpo=
-----END PRIVATE KEY-----
EOF;

$public_key_pem = <<<EOF
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAiC56XD+PUbnjvSvnPM18
XBYNPEc0w1G9PJQcoYNzZnBEiSXqnjhPNkSu6V4qQVf8CHJOSSi5gsJkeyS4HQRH
73QoKCRk+k0yyNMmYDZBKDrPkZxN83ORGjssz0AHGQV6Z4fdIs64AU8qes2u+4sJ
0oS9/2wVWzuXTXF2MWiARXd4JBySx2da7ERIdbwcv29tO62W7TS+o2M2ARV7xJuI
moxfLr2ViLUfVUALLFDQw34SyKfgzxAyiWanqgR/Jw7CWTpd+jWoOb0TxSlcD87b
YZTXPvxHt4U4X+a2lm/HyFUrOxJEJLLmcpzs6aEsJx1pp77S+22Zr2nleHyQ4PEE
CQIDAQAB
-----END PUBLIC KEY-----
EOF;

$certificate_pem = <<<EOF
-----BEGIN CERTIFICATE-----
MIIDCzCCAfOgAwIBAgIUPjMQAIaOIt6dvBkm0fbknjJST90wDQYJKoZIhvcNAQEL
BQAwFDESMBAGA1UEAwwJa3BocC10ZXN0MCAXDTI2MTEyNjA5MTM0M1oYDzIxMjYx
MTAyMDkxMzQzWjAUMRIwEAYDVQQDDAlrcGhwLXRlc3QwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQCILnpcP49RueO9K+c8zXxcFg08RzTDUb08lByhg3Nm
cESJJeqeOE82RK7pXipBV/wIck5JKLmCwmR7JLgdBEfvdCgoJGT6TTLI0yZgNkEo
Os+RnE3zc5EaOyzPQAcZBXpnh90izrgBTyp6za77iwnShL3/bBVbO5dNcXYxaIBF
d3gkHJLHZ1rsREh1vBy/b207rZbtNL6jYzYBFXvEm4iajF8uvZWItR9VQAssUNDD
fhLIp+DPEDKJZqeqBH8nDsJZOl36Nag5vRPFKVwPztthlNc+/Ee3hThf5raWb8fI
VSs7EkQksuZynOzpoSwnHWmnvtL7bZmvaeV4fJDg8QQJAgMBAAGjUzBRMB0GA1Ud
DgQWBBQrE8rPipwa5iUIQqf+iSG+nhuvizAfBgNVHSMEGDAWgBQrE8rPipwa5iUI
Qqf+iSG+nhuvizAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQBG
SmZ95NmG8sUDznOzTxPiOcl3iWZfvAFNgzS0rIdpzhYvnk+VpLnMHq9uZCZoIP8p
yzWKHhXbDMCrk7G0+7FK0mj3mVW24z/mFytZokHV8hi7GssE00KL4EVO0tfqc3WY
aUPVoylSZ8TCvHwRbCvn3SsO94teAsBGwMCDmBkaalUnXF55/y2wB2OYNUwmmdfU
skl3sC28rVpIB8nvZ/AGkks0mDqYRbBN5lyKgiP6F+zZBy/ivYhrcujZPXkkK1FC
7YdXTDt/MKcpLURMbevTm/q8eCZhUDqCS13i9xA36jjvL8yrH00SBOSnZcaQnih/
rExpb2VlxkGKy0jEeV7P
-----END CERTIFICATE-----
EOF;

$ec_private_key_pem = <<<EOF
-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgOVjtTVNfJ4wmDk/5
SiS1fb174c/zUEwc7IaUwHeehKShRANCAATM1nnOmcSr/DneuDayktQDCa1wx1GG
2HOlYaKeKiAD9J76tatxLNIdflGxy1J0xX0HeIID+RnQKbP8BvDldhnb
-----END PRIVATE KEY-----
EOF;

$ec_public_key_pem = <<<EOF
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEzNZ5zpnEq/w53rg2spLUAwmtcMdR
hthzpWGiniogA/Se+rWrcSzSHX5RsctSdMV9B3iCA/kZ0Cmz/Abw5XYZ2w==
-----END PUBLIC KEY-----
EOF;

function test_rsa_basic_pem() {
    global $private_key_pem, $public_key_pem;

    $data = 'RSA test data';
    $encrypted = '';
    var_dump(openssl_public_encrypt($data, $encrypted, $public_key_pem));
    var_dump(strlen($encrypted));

    $decrypted = '';
    var_dump(openssl_private_decrypt($encrypted, $decrypted, $private_key_pem));
    var_dump($decrypted);
}

function test_rsa_handles() {
    global $private_key_pem, $public_key_pem;

    $private_handle = openssl_pkey_get_private($private_key_pem);
    var_dump($private_handle !== false);

    $pub_handle = openssl_pkey_get_public($public_key_pem);
    var_dump($pub_handle !== false);

    $data = 'RSA with handles';
    $encrypted = '';
    var_dump(openssl_public_encrypt($data, $encrypted, $pub_handle));
    var_dump(strlen($encrypted));

    $decrypted = '';
    var_dump(openssl_private_decrypt($encrypted, $decrypted, $private_handle));
    var_dump($decrypted);

    $data2 = 'Second use of same handle';
    $encrypted2 = '';
    var_dump(openssl_public_encrypt($data2, $encrypted2, $pub_handle));
    var_dump(strlen($encrypted2));
    $decrypted2 = '';
    var_dump(openssl_private_decrypt($encrypted2, $decrypted2, $private_handle));
    var_dump($decrypted2);
}

function test_handle_roundtrip() {
    global $private_key_pem, $public_key_pem;

    $handle = openssl_pkey_get_private($private_key_pem);
    var_dump($handle !== false);

    $data = 'roundtrip test';
    $encrypted = '';
    var_dump(openssl_public_encrypt($data, $encrypted, openssl_pkey_get_public($public_key_pem)));
    $decrypted = '';
    var_dump(openssl_private_decrypt($encrypted, $decrypted, $handle));
    var_dump($decrypted);
}

function test_rsa_empty_data() {
    global $private_key_pem, $public_key_pem;

    $encrypted = '';
    var_dump(openssl_public_encrypt('', $encrypted, $public_key_pem));
    var_dump(strlen($encrypted));
    $decrypted = '';
    var_dump(openssl_private_decrypt($encrypted, $decrypted, $private_key_pem));
    var_dump($decrypted);
}

function test_rsa_long_data() {
    global $private_key_pem, $public_key_pem;

    // Max data for 2048-bit RSA with PKCS#1 v1.5 padding: 256 - 11 = 245 bytes
    $long_data = str_repeat('A', 245);
    $encrypted = '';
    var_dump(openssl_public_encrypt($long_data, $encrypted, $public_key_pem));
    var_dump(strlen($encrypted));
    $decrypted = '';
    var_dump(openssl_private_decrypt($encrypted, $decrypted, $private_key_pem));
    var_dump($decrypted);

    // Data exactly 246 bytes (too long for PKCS#1 v1.5)
    $too_long = str_repeat('A', 246);
    $encrypted2 = '';
    var_dump(openssl_public_encrypt($too_long, $encrypted2, $public_key_pem));
}

function test_rsa_mixed() {
    global $private_key_pem, $public_key_pem;

    $pub = openssl_pkey_get_public($public_key_pem);
    $data = 'mixed test';
    $encrypted = '';
    var_dump(openssl_public_encrypt($data, $encrypted, $pub));
    var_dump(strlen($encrypted));

    // Decrypt with PEM string (not handle)
    $decrypted = '';
    var_dump(openssl_private_decrypt($encrypted, $decrypted, $private_key_pem));
    var_dump($decrypted);
}

function test_rsa_errors() {
    $err_result = '';
    var_dump(openssl_public_encrypt('data', $err_result, 'invalid key'));
    var_dump(openssl_private_decrypt('data', $err_result, 'invalid key'));
    var_dump(openssl_public_encrypt('data', $err_result, ''));
    var_dump(openssl_private_decrypt('data', $err_result, ''));
}

function test_certificate_as_public_key() {
    global $private_key_pem, $certificate_pem;

    $encrypted = '';
    var_dump(openssl_public_encrypt('encrypted with certificate', $encrypted, $certificate_pem));
    var_dump(strlen($encrypted));
    $decrypted = '';
    var_dump(openssl_private_decrypt($encrypted, $decrypted, $private_key_pem));
    var_dump($decrypted);

    $cert_handle = openssl_pkey_get_public($certificate_pem);
    var_dump($cert_handle !== false);
    $encrypted2 = '';
    var_dump(openssl_public_encrypt('encrypted with certificate handle', $encrypted2, $cert_handle));
    var_dump(strlen($encrypted2));
    $decrypted2 = '';
    var_dump(openssl_private_decrypt($encrypted2, $decrypted2, $private_key_pem));
    var_dump($decrypted2);
}

function test_wrong_key_type() {
    global $private_key_pem, $public_key_pem;

    // a private key is not accepted where a public key is expected
    $result = '';
    var_dump(openssl_public_encrypt('data', $result, $private_key_pem));

    // and vice versa
    $encrypted = '';
    var_dump(openssl_public_encrypt('data', $encrypted, $public_key_pem));
    $result = '';
    var_dump(openssl_private_decrypt($encrypted, $result, $public_key_pem));
}

function test_wrong_handle_kind() {
    global $private_key_pem, $public_key_pem;

    $private_handle = openssl_pkey_get_private($private_key_pem);
    $pub_handle = openssl_pkey_get_public($public_key_pem);

    $result = '';
    var_dump(openssl_public_encrypt('data', $result, $private_handle));

    $encrypted = '';
    var_dump(openssl_public_encrypt('data', $encrypted, $pub_handle));
    $result = '';
    var_dump(openssl_private_decrypt($encrypted, $result, $pub_handle));
}

function test_ec_keys() {
    global $ec_private_key_pem, $ec_public_key_pem;

    // only RSA keys can be used for encryption
    $result = '';
    var_dump(openssl_public_encrypt('data', $result, $ec_public_key_pem));

    $result = '';
    var_dump(openssl_private_decrypt(str_repeat('a', 32), $result, $ec_private_key_pem));

    $ec_pub_handle = openssl_pkey_get_public($ec_public_key_pem);
    var_dump($ec_pub_handle !== false);
    $result = '';
    var_dump(openssl_public_encrypt('data', $result, $ec_pub_handle));
}

function test_binary_data() {
    global $private_key_pem, $public_key_pem;

    $data = "\0\1\2\xff\0";
    $encrypted = '';
    var_dump(openssl_public_encrypt($data, $encrypted, $public_key_pem));
    $decrypted = '';
    var_dump(openssl_private_decrypt($encrypted, $decrypted, $private_key_pem));
    var_dump(bin2hex($decrypted));

    $all_bytes = '';
    for ($i = 0; $i < 245; ++$i) {
        $all_bytes .= chr($i);
    }
    
    var_dump(openssl_public_encrypt($all_bytes, $encrypted, $public_key_pem));
    var_dump(openssl_private_decrypt($encrypted, $decrypted, $private_key_pem));
    var_dump($decrypted === $all_bytes);
}

test_rsa_basic_pem();
test_rsa_handles();
test_handle_roundtrip();
test_rsa_empty_data();
test_rsa_long_data();
test_rsa_mixed();
test_rsa_errors();
test_certificate_as_public_key();
test_wrong_key_type();
test_wrong_handle_kind();
test_ec_keys();
test_binary_data();
