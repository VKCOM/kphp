@ok
<?php

function test_yaml_parse_config_file() {
  /** @param string $filename */
  $filename = dirname(__FILE__) . '/../../../docs/_config.yml';
  /** @param mixed $in */
  $in = yaml_parse_file($filename);
  echo(serialize($in) . PHP_EOL);
  /** @param mixed $out */
  $out = yaml_parse(yaml_emit($in));
  echo(serialize($out) . PHP_EOL);
}

test_yaml_parse_config_file();
