<?php

require_once 'vendor/autoload.php';

use VK\Ads\Controller;
use VK\Ads;
use MultiDir\MultiClass1; // from ./multi1
use MultiDir\MultiClass2; // from ./multi2
use Fallback1\Fallback2\FallbackClass; // from ./fallback
use Fallback1\Fallback2\my_fallback; // from ./fallback

class AnotherController extends Ads\Helpers\BaseController {}

new MultiClass1();
new MultiClass2();
new FallbackClass();
new my_fallback();

$controller = new Controller();
$controller->init();

$my_controller = new Ads\my_controller();
$my_controller->init();

$my_pretty_controller = new Ads\my_pretty_controller();
$my_pretty_controller->init();

echo "alright\n";
