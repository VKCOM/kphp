<?php

$ch = curl_init();

curl_setopt($ch, CURLOPT_URL, "nonexistent_url");
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

$output = curl_exec($ch);

echo $output;

curl_close($ch);