<?php

$ch = curl_init();

curl_setopt($ch, CURLOPT_URL, "nonexistent_url");

curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

$output = curl_exec($ch);
	
var_dump($output);

curl_close($ch);
