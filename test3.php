<?php

$ch = curl_init();

curl_setopt($ch, CURLOPT_URL, "nonexistent_url");

// curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

$output = curl_exec_concurrently($ch, 1);
	
var_dump($output);

curl_close($ch);
