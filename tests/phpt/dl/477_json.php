@ok benchmark k2_skip
<?php

function test_json ($options) {
  $php_ver = 7;
#ifndef KPHP
    $php_ver = (int)phpversion();
#endif

  var_dump (json_encode (NULL, $options));
  var_dump (json_encode (true, $options));
  var_dump (json_encode (false, $options));
  var_dump (json_encode (.001234, $options));
  var_dump (json_encode (-0.012340, $options));
  var_dump (json_encode (-0.012341, $options));
  var_dump (json_encode (-0.012342, $options));
  var_dump (json_encode (-0.012343, $options));
  var_dump (json_encode (-0.012344, $options));
  var_dump (json_encode (-0.012345, $options));
  var_dump (json_encode (-0.012346, $options));
  var_dump (json_encode (-0.012347, $options));
  var_dump (json_encode (-0.012348, $options));
  var_dump (json_encode (-0.012349, $options));
  var_dump (json_encode (-0.012350, $options));
  var_dump (json_encode (-0.012351, $options));
  var_dump (json_encode (-0.012352, $options));
  var_dump (json_encode (-0.012353, $options));
  var_dump (json_encode (-0.012354, $options));
  var_dump (json_encode (-0.012355, $options));
  var_dump (json_encode (-0.012356, $options));
  var_dump (json_encode (-0.012357, $options));
  var_dump (json_encode (-0.012358, $options));
  var_dump (json_encode (-0.012359, $options));
  var_dump (json_encode (1e800, $options));
  var_dump (json_encode (-0.2, $options));
  var_dump (json_encode (-0.5, $options));
  var_dump (json_encode (-0.123456789, $options));
//  var_dump (json_encode (-0.123456789123456789, $options));
//  var_dump (json_encode (123456789123456789123456789.0, $options));
  var_dump (json_encode (-1, $options));
  var_dump (json_encode (-0, $options));
  var_dump (json_encode (0, $options));
  var_dump (json_encode (1, $options));
  var_dump (json_encode (112312312, $options));
  var_dump (json_encode (11231, $options));
  var_dump (json_encode ("", $options));
  var_dump (json_encode ("12312312", $options));
  var_dump (json_encode ("/a/b/\c//d\/e", $options));
  var_dump (json_encode (array(), $options));
  var_dump (json_encode (array(1, 2, 3), $options));
  var_dump (json_encode (array(-1 => 1, 2, 3), $options));
  var_dump (json_encode (array(-1 => 1, "aas  da,sd" => array("2," => array (3, 4, 5)), 3, NULL, true, 0.001234, 123, "\"{\t\r\n[]}\""), $options));

  $a = array('<foo>',"'bar'",'"baz"','&blong&', "\xc2\xa2", "\xc3\xa9", "\xF4\x8F\xBF\xBF", "\xF4\x90\x80\x80", "\xc3\xa9\xF4\x8F\xBF\xBF\xF4\x90\x80\x80",
             "\xD0\x9A", "\xE0\x90\x9A", "\xF0\x80\x90\x9A", "\xF8\x80\x80\x90\x9A", "\xFC\x80\x80\x80\x90\x9A",
             "\xE0\xAF\xB5", "\xED\x9F\xBF", "\xED\xA0\x80", "\xED\xBF\xBF", "\xEE\x80\x80", "\xF0\xA6\x88\x98", "\xF4\x8F\xBF\xBF",
             "\xF4\x90\x80\x80", "\xF8\xAA\xBC\xB7\xAF", "\xFD\xBF\xBF\xBF\xBF\xBF");
  $b = range ("\x00", "\x7f");
  @var_dump (json_encode ($a, $options)); // can't use JSON_UNESCAPED_UNICODE due to bug in PHP, input isn't validated
  var_dump (json_encode ($b, $options));
  @var_dump (json_decode (json_encode ($a, $options)), true); // can't use JSON_UNESCAPED_UNICODE due to bug in PHP, input isn't validated
  var_dump (json_decode (json_encode ($b, $options)), true);

  var_dump (json_decode ('["<foo>","\'bar\'","\"baz\"","&blong&","\u00e9","\udbff\udfff",null]'));
  if ($php_ver >= 7) {
      var_dump (json_decode ('["<\xfoo>","\'bar\'","\"baz\"","&blong&","\u00e9","\udbff\udfff",null]'));
      var_dump (json_decode ('["<foo>","\'bar\'","\"baz\"","&blong&","\udbff","\udbff\udfff",null]'));
      var_dump (json_decode ('["<foo>","\'bar\'","\"baz\"","&blong&","\u00e9","\udbff\ud0ff",null]'));
  } else {
      var_dump(NULL);
      var_dump(NULL);
      var_dump(NULL);
  }

  var_dump (json_decode (json_encode (NULL, $options), true));
  var_dump (json_decode (json_encode (true, $options), true));
  var_dump (json_decode (json_encode (false, $options), true));
  var_dump (json_decode (json_encode (.001234, $options), true));
  var_dump (json_decode (json_encode (-0.012346, $options), true));
  var_dump (json_decode (json_encode (-1, $options), true));
  var_dump (json_decode (json_encode (-0, $options), true));
  var_dump (json_decode (json_encode (0, $options), true));
  var_dump (json_decode (json_encode (1, $options), true));
  var_dump (json_decode (json_encode (112312312, $options), true));
  var_dump (json_decode (json_encode (11231, $options), true));
  var_dump (json_decode (json_encode ("", $options), true));
  var_dump (json_decode (json_encode ("12312312", $options), true));
  var_dump (json_decode (json_encode (array(), $options), true));
  var_dump (json_decode (json_encode (array(1, 2, 3), $options), true));
  var_dump (json_decode (json_encode (array(-1 => 1, 2, 3), $options), true));
  var_dump (json_decode (json_encode (array(-1 => 1, "aa  sda,sd" => array("2," => array (3, NULL, false)), 3, NULL, true, 0.001234, 123, "\"{\t\r\n[]}\""), $options), true));

  var_dump (json_encode (array ("0" => "0"), $options));

  $json = '  { "a" : 1     ,    "b"   :   2   ,   "c"  :3,  "d":4  ,"e":5   }   ';
  var_dump (json_decode($json, true));

  $json = '  [ 1     ,    "b"   ,   2   ,   "c"  ,3,  "d",4  ,"e",5   ]   ';
  var_dump (json_decode($json, true));

  $json = '{"foo-bar": 12345}';

  $obj = json_decode($json, true);
  print $obj['foo-bar']."\n";

  $bad_json = "{ 'bar': 'baz' }";
  var_dump (json_decode($bad_json));

  $bad_json = '{ bar: "baz" }';
  var_dump (json_decode($bad_json));

  $bad_json = '{ bar: "baz", }';
  var_dump (json_decode($bad_json));

  var_dump (json_decode('{"-1":1,"aasda,sd":{"2,":[3,null,false]},"0":3,"1":null,"2":true,"3":0.001234,"4":123,"5":"\\u0123\\u0039\\"{\\t\\r\\n[]}\\""}', true));


  $data1 = array(1, 1.01, 'hello world', true, null, -1, 11.011, '~!@#$%^&*()_+|', false, null);
  $data2 = array('zero' => $data1, 'one' => $data1, 'two' => $data1, 'three' => $data1, 'four' => $data1, 'five' => $data1, 'six' => $data1, 'seven' => $data1, 'eight' => $data1, 'nine' => $data1);
  $data = array($data2, $data2, $data2, $data2, $data2, $data2, $data2, $data2, $data2, $data2);

  for ($i = 0; $i < 1; $i++) {
    $data_encoded = json_encode ($data, $options);
  }

  for ($i = 0; $i < 5000; $i++) {
    $data = json_decode ($data_encoded);
  }

  var_dump (count($data));
}

test_json (0);
test_json (JSON_UNESCAPED_UNICODE);
test_json (JSON_UNESCAPED_SLASHES);
test_json (JSON_PARTIAL_OUTPUT_ON_ERROR);
