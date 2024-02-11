@ok
<?php

function test_yaml_complex() {
  /** @param mixed $in */
  $in = array("10", 10, "9.7", 9.7, "", array(), array("true"=>"true", true=>true, false=>false, array("false"=>"false", 10 => null)));
  echo(serialize($in) . PHP_EOL);
  /** @param mixed $out */
  $out = yaml_parse(yaml_emit($in));
  echo(serialize($out) . PHP_EOL);
}

test_yaml_complex();
