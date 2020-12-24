<?php

require_once 'vendor/autoload.php';

use VK\Utils\Strings;
use VK\Common\Pkg1\C;
use VK\Feed\Controller;
use VK\Feed;
use VK\Feed\FeedTester; // available only when building with autoload-dev
use MultiDir\MultiClass1; // from ./multi1
use MultiDir\MultiClass2; // from ./multi2
use Fallback1\Fallback2\FallbackClass; // from ./fallback

class MyController extends Feed\Helpers\BaseController {}

new C();
new MultiClass1();
new MultiClass2();
new FallbackClass();
new UtilsFallback(); // from ./packages/utils/utils-fallback/src

$controller = new Controller();

$t = new FeedTester();

echo json_encode([
  'startsWith' => Strings::startsWith("foo", "fo"),
  'init' => $controller->init(),
]);
