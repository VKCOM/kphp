@ok k2_skip
<?php

function make_tests_data(): array {
  $data = [
    [
      'key' => '00000000000000000000000000000000',
      'iv'  => '000000000000000000000000',
      'tag' => '58e2fccefa7e3061367f1d57a4e7455a',
      'pt'  => '',
      'ct'  => '',
      'aad' => ''
    ],
    [
      'key' => '00000000000000000000000000000000',
      'iv'  => '000000000000000000000000',
      'tag' => 'ab6e47d42cec13bdf53a67b21257bddf',
      'pt'  => '00000000000000000000000000000000',
      'ct'  => '0388dace60b6a392f328c2b971b2fe78',
      'aad' => ''
    ],
    [
      'key' => 'feffe9928665731c6d6a8f9467308308',
      'iv'  => 'cafebabefacedbaddecaf888',
      'tag' => '4d5c2af327cd64a62cf35abd2ba6fab4',
      'pt'  => 'd9313225f88406e5a55909c5aff5269a' .
               '86a7a9531534f7da2e4c303d8a318a72' .
               '1c3c0c95956809532fcf0e2449a6b525' .
               'b16aedf5aa0de657ba637b391aafd255',
      'ct'  => '42831ec2217774244b7221b784d0d49c' .
               'e3aa212f2c02a4e035c17e2329aca12e' .
               '21d514b25466931c7d8f6a5aac84aa05' .
               '1ba30b396a0aac973d58e091473f5985',
      'aad' => ''
    ],
    [
      'key' => 'feffe9928665731c6d6a8f9467308308',
      'iv'  => 'cafebabefacedbaddecaf888',
      'aad' => 'feedfacedeadbeeffeedfacedeadbeefabaddad2',
      'tag' => '5bc94fbc3221a5db94fae95ae7121a47',
      'pt'  => 'd9313225f88406e5a55909c5aff5269a' .
               '86a7a9531534f7da2e4c303d8a318a72' .
               '1c3c0c95956809532fcf0e2449a6b525' .
               'b16aedf5aa0de657ba637b39',
      'ct'  => '42831ec2217774244b7221b784d0d49c' .
               'e3aa212f2c02a4e035c17e2329aca12e' .
               '21d514b25466931c7d8f6a5aac84aa05' .
               '1ba30b396a0aac973d58e091',
    ],
    [
      'key' => 'feffe9928665731c6d6a8f9467308308',
      'iv'  => 'cafebabefacedbad',
      'aad' => 'feedfacedeadbeeffeedfacedeadbeefabaddad2',
      'tag' => '3612d2e79e3b0785561be14aaca2fccb',
      'pt'  => 'd9313225f88406e5a55909c5aff5269a' .
               '86a7a9531534f7da2e4c303d8a318a72' .
               '1c3c0c95956809532fcf0e2449a6b525' .
               'b16aedf5aa0de657ba637b39',
      'ct'  => '61353b4c2806934a777ff51fa22a4755' .
               '699b2a714fcdc6f83766e5f97b6c7423' .
               '73806900e49f24b22b097544d4896b42' .
               '4989b5e1ebac0f07c23f4598'
    ],
    [
      'key' => 'feffe9928665731c6d6a8f9467308308',
      'iv'  => '9313225df88406e555909c5aff5269aa' .
               '6a7a9538534f7da1e4c303d2a318a728' .
               'c3c0c95156809539fcf0e2429a6b5254' .
               '16aedbf5a0de6a57a637b39b',
      'aad' => 'feedfacedeadbeeffeedfacedeadbeefabaddad2',
      'tag' => '619cc5aefffe0bfa462af43c1699d050',
      'pt'  => 'd9313225f88406e5a55909c5aff5269a' .
               '86a7a9531534f7da2e4c303d8a318a72' .
               '1c3c0c95956809532fcf0e2449a6b525' .
               'b16aedf5aa0de657ba637b39',
      'ct'  => '8ce24998625615b603a033aca13fb894' .
               'be9112a5c3a211a8ba262a3cca7e2ca7' .
               '01e4a9a4fba43c90ccdcb281d48c7c6f' .
               'd62875d2aca417034c34aee5',
    ],
  ];

  $result = [];
  foreach ($data as $test) {
    $encoded = [];
    foreach ($test as $key => $value) {
      $encoded[$key] = pack("H*", $value);
    }
    $result[] = $encoded;
  }
  return $result;
}

function test_decrypt() {
  $method = "aes-128-gcm";

  foreach (make_tests_data() as $idx => $test) {
    echo "TEST $idx\n";
    $pt = openssl_decrypt($test['ct'], $method, $test['key'], OPENSSL_RAW_DATA, $test['iv'], $test['tag'], $test['aad']);
    var_dump($test['pt'] === $pt);

    // no IV
    var_dump(openssl_decrypt($test['ct'], $method, $test['key'], OPENSSL_RAW_DATA, "", $test['tag'], $test['aad']));
    // failed because no AAD
    var_dump(openssl_decrypt($test['ct'], $method, $test['key'], OPENSSL_RAW_DATA, $test['iv'], $test['tag']));
    // failed because wrong tag
    var_dump(openssl_decrypt($test['ct'], $method, $test['key'], OPENSSL_RAW_DATA, $test['iv'], str_repeat('x', 10), $test['aad']));
  }
}

function test_encrypt() {
  $method = "aes-128-gcm";

  foreach (make_tests_data() as $idx => $test) {
    echo "TEST $idx\n";
    $tag = '';
    $ct = openssl_encrypt($test['pt'], $method, $test['key'], OPENSSL_RAW_DATA, $test['iv'], $tag, $test['aad'], strlen($test['tag']));
    var_dump($test['ct'] === $ct);
    var_dump($test['tag'] === $tag);

    // Empty IV error
    var_dump(openssl_encrypt('data', $method, 'password', 0, NULL, $tag, ''));

    // Test setting different IV length and tag length
    var_dump(openssl_encrypt('data', $method, 'password', 0, str_repeat('x', 10), $tag, '', 14));
    var_dump(strlen($tag));

    $tag = 'hello world';
    // Failing to retrieve tag (max is 16 bytes)
    var_dump(openssl_encrypt('data', $method, 'password', 0, str_repeat('x', 32), $tag, '', 20));
    var_dump($tag);

    // Test setting invalid tag length
    var_dump(openssl_encrypt('data', $method, 'password', 0, str_repeat('x', 16), $tag, '', 1024));
    var_dump($tag);

    // Failing when no tag supplied
    var_dump(openssl_encrypt('data', $method, 'password', 0, str_repeat('x', 32)));
  }
}

test_decrypt();
test_encrypt();
