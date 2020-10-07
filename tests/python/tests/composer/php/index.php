<?php

#ifndef KPHP
require_once './vendor/autoload.php';
#endif

use VK\Utils\Strings;
use VK\Common\Pkg1\C;
use VK\Feed\Controller;
use VK\Feed;
use VK\Feed\FeedTester; // available only when building with autoload-dev

class MyController extends Feed\Helpers\BaseController {}

new C();

$controller = new Controller();

$t = new FeedTester();

echo json_encode([
  'startsWith' => Strings::startsWith("foo", "fo"),
  'init' => $controller->init(),
]);
