<?php

$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, "nonexistent_url");

curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

$output = curl_exec($ch);
	
var_dump($output);


function to_json_safe($str) {
  return is_string($str) && $str != '' ? json_decode($str) : $str;
}

$resp = ["exec_result" => to_json_safe($output)];
var_dump($resp);


curl_close($ch);
