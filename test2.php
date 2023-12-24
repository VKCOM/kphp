<?php

$ch = curl_init();

curl_setopt($ch, CURLOPT_URL, "http://example.com");

curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

$output = curl_exec($ch);
	
var_dump($output);

curl_close($ch);
