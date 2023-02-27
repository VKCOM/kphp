@ok
<?php

function test_emit_parse_complex_mixed() {
  /** @param mixed $in */
  $in = array("10", 10, "9.7", 9.7, "", array("true"=>"true", true=>true, false=>false, array("false"=>"false", 10 => null)));
  echo(serialize($in) . PHP_EOL);
  /** @param mixed $out */
  $out = yaml_parse(yaml_emit($in));
  echo(serialize($out) . PHP_EOL);
}

test_emit_parse_complex_mixed();