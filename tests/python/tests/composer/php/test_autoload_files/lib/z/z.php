<?php

require "vendor/" . "autoload.php";

global $global_map;
$global_map[__FILE__] = true;

function z_test() {
  var_dump("z" . vk_api_version());
  return "ok";
}
