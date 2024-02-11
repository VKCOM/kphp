@ok
<?php

function test_yaml_escaping() {
  /** @param string $filename */
  $filename = dirname(__FILE__) . '/yaml_escaping.yml';
  /** @param mixed $in */
  $in = yaml_parse_file($filename);
  echo(serialize($in) . PHP_EOL);
//   echo(yaml_emit($in) . PHP_EOL); // does not pass because PHP YAML does not use double quotes sometimes
  /** @param mixed $out */
  $out = yaml_parse(yaml_emit($in));
  echo(serialize($out) . PHP_EOL);
}

test_yaml_escaping();

