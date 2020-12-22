<?php

/** @kphp-immutable-class */
class A {
  /** @var int */
  public $a = 42;
  /** @var string */
  public $b = "hello";
}

if ($_SERVER["PHP_SELF"] === "/ini_get") {
  echo ini_get($_SERVER["QUERY_STRING"]);
} else if ($_SERVER["PHP_SELF"] === "/sleep") {
  $sleep_time = (int)$_GET["time"];
  echo "before sleep $sleep_time\n";
  fwrite(STDERR, "sleeping for $sleep_time sec ...\n");
  sleep($sleep_time);
  fwrite(STDERR, "wake up!");
  echo "after sleep";
} else if ($_SERVER["PHP_SELF"] === "/store-in-instance-cache") {
  echo instance_cache_store("test_key" . rand(), new A);
} else {
  echo "Hello world!";
}
