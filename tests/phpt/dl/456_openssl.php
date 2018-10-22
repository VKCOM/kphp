@ok openssl
<?php

$ads_public_key = <<<EOF
-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDRG9NNNLsO+vDEDh1hHealSQFe
LG/0Q/0Fr9lr7VP75YifZHy0BMeq04HP2XBxgVsdTHfcjR0kqlA7nyukUbdClxbZ
lEhMee1IlSVskYGct1+iOGtwYjKVcjxx/XpafOrRV3gUz3ByI0vIuW6T0xO9fq63
JnUc1qVXZrv9O8E83QIDAQAB
-----END PUBLIC KEY-----
EOF;

$ads_private_key = <<<EOF
-----BEGIN RSA PRIVATE KEY-----
MIICXAIBAAKBgQDRG9NNNLsO+vDEDh1hHealSQFeLG/0Q/0Fr9lr7VP75YifZHy0
BMeq04HP2XBxgVsdTHfcjR0kqlA7nyukUbdClxbZlEhMee1IlSVskYGct1+iOGtw
YjKVcjxx/XpafOrRV3gUz3ByI0vIuW6T0xO9fq63JnUc1qVXZrv9O8E83QIDAQAB
AoGAKzO4gCb4zquBur+/yiTHVjazFaXQq4Kwb9BY9zddNpnBlEzKhtbF+cEg/LRL
yueSz4bQ7Lwq3Txivy6vcY+AcN2utCOmQmxUt0+iKgPbrTg2dEuhjq7E2x15Nq1u
k5L+9WccFddGGrwFPZRYct4b9jJv9o8SlnXtbUcSYXdXXAECQQD5a0mBLYrb3tDQ
plFo39mSfs5zwZ2UIWZj0Kox1cO57TzR+glKF0A9d2P4Q8yWTJCBxmPNJypzMeLk
0E0VInedAkEA1qBCgvz+07Fgd7TYpMfbuaTroI3MJL7KH/XfuC7RRJGTf8rKChOA
9tiskin2FAlrfJobjcS6pB1n3kTEdlD2QQJAO8vc83tXrx7cMSmumtYP828zT807
WignxAZix0/YfNrDmhO35mtsm0/kR8D5a48vle1aP/UD2Fo9a14FHOwGgQJBAJIM
ylYCvZHm0WOEUCyJyC7jWGiQNYwHvNnU4iGe0k8b1UiQAb6rmQXhWkW4gjkOU/Od
lwR5DB0j2Yu6Ngrhe0ECQHj3ohNqKZcarIZNpYmwgxwhS7lnSjymtz3tlh0I62yw
5Ver0+OQ3rrJcBhNvi9sapRQtMc+mjVKT5ut3mxkNh4=
-----END RSA PRIVATE KEY-----
EOF;


$s = "Hello world!!!";

var_dump (openssl_public_encrypt ($s, $result, $ads_public_key));

var_dump (openssl_private_decrypt ($result, $new_s, $ads_private_key));

var_dump ($new_s);


$SSLPubKey = <<<EOF
-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDFsQxdtrqd1tj3CZ0KzNQyxnYI
46u4wAl3WvTdielihbDsnEzU4rIifdTB3gop64d95CWNr3oEoxntSXqWGHkHgV9l
gAu2Q/kwoC/+eJY+ZzWQHcXNkZg6IbZJEvdkn7Y770u5K17CsI6UgNGCGd22UXGn
ekyjRCQSkzlMr1mDbQIDAQAB
-----END PUBLIC KEY-----
EOF;

$data = "Hello world!!!";

var_dump (openssl_public_encrypt ($data, $crypted, $SSLPubKey));
//var_dump ($crypted);



$SSLPrivKey = <<<EOF
-----BEGIN RSA PRIVATE KEY-----
Proc-Type: 4,ENCRYPTED
DEK-Info: DES-EDE3-CBC,B97671A4D8214B34

cZSxPDNcgID6SUtSHeiiNwJTkwryVCPKvdRkg8H/i7VSgd232Fr+kmtu0ODQ2Nuo
TSUi0eB+5LJEqXK3Mk65dIiiaSXgwtR7fKCu+QkhhGkWxa30tnQMF3sv3A2jKFW7
354YHiEKZZkiXeaWHIknzM5dMHkzhhHfzbBwRhOh6PhmzgS6VACRAKSmREUp/np1
hjYyX4Q79nAOyuqZBncVEUjU8v4nRZP7KGF+C5qsOysSjuTAhBLr4jJNT6Byo5hq
cByDZ7cR/9S53va2osuiRqhIy4hV8Nepl1AwZ9M+AL/Hv2KmQDxcQM4bvgxDuNz9
hnELsRS3wOyGpXSSqLOdXBGnG3unqakE9eE8+ha4kk9tfgrJQlZqAYo3fk8Vijy1
HlZRK9FfdD8LudYzBCb+R9xOTmqhaPtXe/xd8hvLi4JjnxVg6C9OHXmbEj05OrNO
nIFkKnmKscC1UhO+8Vdi12HGliBD0jcEcY512xxxOvzHegNyPvso4P1E9n34tTer
Ff0zwKY/Hvw7MSytBQjNa72Sm2vRtckFXhtbk5JdeSwAINNyB6INGYYs/5D22dLN
wWA5ZT2MN7SZLOqR5fElz68sIbjPotrQbfvGZTrxA5riVBY8O6YijrdoX+pgtzEX
Ris4l+DdelyAf0jni5j2pMNPTsHFElZl5LDTrxshMy7D584gpms4RC6rcCCdnQYd
ytXikzjQ/nmj6ZilwpE3xWbq5muqCkvUYfXIB/QSVX0/Bi0Gxw16iHd4SxENaqn5
vZ9plzkZHi4EG103QspZP6d9B0p94L8/xOGNsrf5yOFFtIxfXhKcwQ==
-----END RSA PRIVATE KEY-----
EOF;

$SSLPass = "SAGER@#*&^)ds3!cew45hfopwn";

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
