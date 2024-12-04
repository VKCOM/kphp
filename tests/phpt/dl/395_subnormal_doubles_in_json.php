@ok
<?php

function test_json_encode() {
  $x = ["xx" => (2 ** -1050)];
  var_dump(json_decode(json_encode($x), true));
}

function test_vk_json_encode() {
  $x = ["xx" => (2 ** -1050)];
  var_dump(vk_json_encode($x));
}

test_json_encode();
test_vk_json_encode();
