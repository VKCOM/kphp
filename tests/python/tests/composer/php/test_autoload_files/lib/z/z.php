<?php

require "vendor/" . "autoload.php";

var_dump(__FILE__ . " required");

function z_test() {
  var_dump("z" . vk_api_version());
  return "ok";
}
