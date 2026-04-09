<?php

require_once 'vendor/autoload.php';

Logger::log("hello");

$client = new Api\ApiClient();
echo $client->request("/users") . "\n";
