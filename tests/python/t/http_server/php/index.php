<?php


if ($_SERVER["PHP_SELF"] === "/ini_get") {
  echo ini_get($_SERVER["QUERY_STRING"]);
} else {
  echo "Hello world!";
}
