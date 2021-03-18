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
} else if ($_SERVER["PHP_SELF"] === "/test_zstd") {
  $res = "";
  switch($_GET["type"]) {
    case "uncompress":
      $res = zstd_uncompress((string)file_get_contents("in.dat")); break;
    case "uncompress_dict":
      $res = zstd_uncompress_dict((string)file_get_contents("in.dat"), (string)file_get_contents($_GET["dict"])); break;
    case "compress":
      $res = zstd_compress((string)file_get_contents("in.dat"), (int)$_GET["level"]); break;
    case "compress_dict":
      $res = zstd_compress_dict((string)file_get_contents("in.dat"), (string)file_get_contents($_GET["dict"])); break;
    default:
      echo "ERROR"; return;
  }
  file_put_contents("out.dat", $res === false ? "false" : $res);
  echo "OK";
} else {
  echo "Hello world!";
}
