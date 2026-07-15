@ok
<?php
function try_to_set_value_if_recv_is_int() {
  /** @var mixed[] $arr */
  $arr = [-1];

  foreach ($arr as $i => $v) {
    // ($arr[$i]['j']) -- recv
    ($arr[$i]['j'])['k'] = true;
  }
}

$try_to_set_value_if_recv_is_string = function () {
  /** @var mixed[] $arr */
  $arr = ["foo"];

  foreach ($arr as $i => $v) {
    // ($arr[$i]['j']) -- recv
    ($arr[$i]['j'])['k'] = true;
  }
};

class C{}

$try_to_set_value_if_recv_is_object = function() {
  /** @var mixed[] $arr */
  $arr = [to_mixed(new C())];

  foreach ($arr as $i => $v) {
    // ($arr[$i]['j']) -- recv
    ($arr[$i]['j'])['k'] = true;
  }
};



try_to_set_value_if_recv_is_int();
#ifndef KPHP
$try_to_set_value_if_recv_is_string = function (){};
$try_to_set_value_if_recv_is_object = function (){};
#endif
$try_to_set_value_if_recv_is_string();
$try_to_set_value_if_recv_is_object();
