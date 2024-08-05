@ok k2_skip
<?php

$ads_public_key = <<<EOF
-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDPbr+P6TX9Qg2glpV77Uz/Vp2G
mZVdJlL0tp6UW/Vx8osZ9Gx//CD+EVzdo3TKH64wYEXZ79agjoSCqN7xJwW1L+QM
ziT6ZVS0dN51+qAd8asjjbuDlcPoJ7w/Jo/WLU4Gu5m/6exM+03G3FByDo7vhd79
GeZBKO9Mr3crhCM2fwIDAQAB
-----END PUBLIC KEY-----
EOF;

$ads_private_key = <<<EOF
-----BEGIN RSA PRIVATE KEY-----
MIICXgIBAAKBgQDPbr+P6TX9Qg2glpV77Uz/Vp2GmZVdJlL0tp6UW/Vx8osZ9Gx/
/CD+EVzdo3TKH64wYEXZ79agjoSCqN7xJwW1L+QMziT6ZVS0dN51+qAd8asjjbuD
lcPoJ7w/Jo/WLU4Gu5m/6exM+03G3FByDo7vhd79GeZBKO9Mr3crhCM2fwIDAQAB
AoGBAJbx1kAYyorSqCv1qC2YDvG3y8WIuWIhYzhkM51uFXunrYVjkhjIFhIL/HHk
YjY7O3xEclAW8S3Ax7h0vlbpuIJDQQalOaLm5tUi0bT8I6/4P5ADrB9cn904Bfyt
TtMbOoZVCop5Wth7KoLtx61M/5kFXo1aTS55+DVffkQkhmJRAkEA6XK3g6IQwegv
k3w6/ZxvcnKd+KBMaCx6gnbz1LqlwjHU6kElLPyouX3KLkrZNkG4qpTc5BNf8hSL
nVoHX26f5QJBAON4p9l98PJbPqA47hkQXvnJF2cnzaZgLoH8Vt1ZjRSfdJDZqh+P
iKed3DDpdoHvtt4VTQHLUUJ/2GFWNHq1bpMCQHsyuTpUmvdaK1FwLEmO9xm09z0w
i2ImpviXAhLv9W5Ikg6WFqJpLDnH8pz/jyYdBPGw4enTd7zvrsZ5ro5keSkCQQCh
/HQbT7JcBFpOovv7YUshOfCuhwvN5UR5UIdTTchH3V2XIUoi+4XnR2Vcd4Tq9xgU
grq6Al21q3Edr9PjZnx9AkEA1DfTgCrwymZ/5OleJHAq4RPCx/BAwF0r2514lJDU
ii8H2zo76ABnoAOV0StdIYzzIypbI7wtBxJmM/KBsx3idA==
-----END RSA PRIVATE KEY-----
EOF;


$s = "Hello world!!!";

var_dump (openssl_public_encrypt ($s, $result, $ads_public_key));

var_dump (openssl_private_decrypt ($result, $new_s, $ads_private_key));

var_dump ($new_s);


$SSLPubKey = <<<EOF
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzSDNXLjWkimBHyAmK4HM
5yy6cA2zx8MyiiqGH4sNe4RE0M7Ac1B1dUST6dCkEgyObk3hl1+CLan1JPJVa7we
a51jN3Yh2gFfqkmj1vDN1GYzCtDQBDDyXChv63gkujPLDTnXfWG6XGL/dA67exDt
TYFMcRL2ZxV9dpvOOMVY6MjaIHwwkfg7xn33pth+LW0uYo/7yZyScuI+utb/y8XS
aYY2gx7SM2G16hvkCunePd45drFRALEfDnXlSfw6nUA/SqhwWA/mrPYpIX08wjOv
GWh1ltVQdHnB2EXGIFVDsaIxvwBbGecgnL0njvs9MP39eJZZBtImSy9apAuE5L0b
TwIDAQAB
-----END PUBLIC KEY-----
EOF;

$data = "Hello world!!!";

var_dump (openssl_public_encrypt ($data, $crypted, $SSLPubKey));
//var_dump ($crypted);



$SSLPrivKey = <<<EOF
-----BEGIN RSA PRIVATE KEY-----
Proc-Type: 4,ENCRYPTED
DEK-Info: AES-256-CBC,CF2C8C85AE5735B452A49E64B4267D5B

uK1lyyRiDBiu39ao1BlZWFEkGf8ytUDivyHzZL979ElTEpGtZuRQBSowI3FSe4CG
2lXbJFjk3SDnHA7oNaW3UgKas2HR//UGrl3lVGOjEGfBKG3m2mBvmwpe2gSl+KZL
hYRvMhdt4uV0QLvmCCes0sV/+zUsVXx0TKbgpM9yq1yxRXLSd2r3Fz/bAFzLPxq6
hBjnTiCiw0iF+rjUNkFuNvCiXY+tRM8+CwqJTsp9r7hopuyIPW5ylIIvB51JZBOA
TYv2ktVeTxQU4VE1K6aMzgMYfoVDlqNJ6DoSnID+kRBMqxpO80S28JZ1XeGBjr+Y
azO2E3RL1f+eHF86dgPvuFuYUo3h+HBFtwPTFEnxDmSqLFfd4BTa1B085bwEiWkP
kvKybAzPp1ua0JSVBYlXw2cWWTaShYx0Qbswe8r3M2n3RLeh86DSIu+andx97I/G
8yyXUAu3GjcYmwPh9WLDPNkvTkVQoswm3HS9xibd1ow1ew4HJodkA2f2XQ9nu60+
YbOQD/nRs51Lu7yLmqzWYv4jfw8dONaSbcm7f52ePSPhcsC57lgN/3MSLuV7BdVh
V8csTcvIfoBrr2EPC1lvQsAxwWRnfh9lEbYztH1OvPwlPIOcXQTVJKs6totJn7tE
uk8j3MUFj6a5Ow8yWuP0wnVtEWWNRYNSP4m/u7WiuBT+QXgjaFqjPRHgiR0WR6ST
X2sh3qA89c2d5gNX+XYUqpQ5hFbiWhpKZS/HcGotYfrFT+4GQZ64fCtSIbly5h7+
A+z4oGa+XlA0Z99nHhiT00s/xQzr/d9BZ/5gDGGp3i1FWo57mVO8yKh5WFKmf/x/
UCcep7nxr3glXsPzW9pJfHxO+Cv76wDtxaHMceRA87/yRfQLaJXeMipG74j+KCEK
oXZK8+2s5rQcnTMz2NevmIJnJXr+FexM3O0wZvmR8HVxKea3fCXpy7OqAfj5dVQw
nokkYf/w3+eTFIs/O0qOY9Jl7OsZs9K8yKB9OZyz2texRUWkdm2Rba4zLxLU6Riq
9UM2GnxdChgvjw/ceRowbJNysY5s984eBIvbBW75kX63hStGAF8ynlm5RD+0LwJW
gGEj3xCFa3NKvXplqwo/TyGp3+fmKPxerS+4Fzjy/V6kd+8eAmIJgxMib+aJaTwF
+q943PPnm/PQCsdDh+vKhIiXx16fY2D+UeM1goFei2hMeIyM3wFbFfsGsje45efj
ADvJYP0cGfe1IEKBVNWu7T2U9F26eZgakTBIpUPUuwfxWk2njc27roBpE6kbc2Sv
rd3RayJYLFZ4laHCbj1o52ngUI4hEXJH1IGc74F++YGGz0oHg4DxsyYDHhDr+LOO
wajIyz59H54+7cSqfZmf5IcY+24pAllDBjOZgekM12v3bt5Wuf/5fHDxuYEoG6N/
fmPPmzNnaIgqtg+bD+W/ntzueZTQAIhZdBohj66/rDAUYpVXJVgW/pd3uqOvVLL1
ctT2+4HrrajVeerT3PWWbnO0MyvycOSlT/weHzE2Lvkd7E11cgkQfN/dSFDN4y3d
HNY/aoRmWgfcWxYoSAbrzslixZEnSbvWHKlY1w2LBSrYztU3qrZxL4a9ZG4fcbRW
-----END RSA PRIVATE KEY-----
EOF;

$SSLPass = "1234567890";

$private_key_pointer = openssl_pkey_get_private ($SSLPrivKey, $SSLPass);

$decrypted = "";
var_dump (openssl_private_decrypt ($crypted, $decrypted, $private_key_pointer));
var_dump ($decrypted);

$public_key_pointer = openssl_pkey_get_public ($SSLPubKey);
$crypted = "";
var_dump (openssl_public_encrypt ($data, $crypted, $public_key_pointer));

$decrypted = "";
var_dump (openssl_private_decrypt ($crypted, $decrypted, $private_key_pointer));
var_dump ($decrypted);
