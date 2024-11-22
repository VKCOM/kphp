@ok
<?php

function encrypt_decrypt(string $method, string $pwd, string $iv, string $text, int $flags)
{
  $encrypted = openssl_encrypt($text, $method, $pwd, $flags, $iv);
  if (!($flags & OPENSSL_RAW_DATA)) {
    var_dump($encrypted);
  }
  $output = openssl_decrypt($encrypted, $method, $pwd, $flags, $iv);
  var_dump($output);
}

function test_basic_encrypt_decrypt()
{
  $pwd_short = "my password";
  $pwd_16 = "This_is_16_bytes";
  $pwd_32 = "This_is_16_bytesThis_is_16_bytes";
  $iv_short = "simple iv";
  $iv_16 = "This_is_16_bytes";

  $text = "hello world! this is my plaintext.";

  foreach (["aes-128-cbc",  "AES-128-CBC", "aes-256-cbc",] as $method) {
    foreach ([$pwd_short, $pwd_16, $pwd_32] as $pwd) {
      foreach ([$iv_short, $iv_16] as $iv) {
        $ivlen = openssl_cipher_iv_length($method);
        if ($ivlen > strlen($iv)) {
          continue;
        }

        encrypt_decrypt($method, $pwd, $iv, $text, OPENSSL_RAW_DATA);
        encrypt_decrypt($method, $pwd, $iv, $text, 0);
      }
    }
  }
}

test_basic_encrypt_decrypt();
