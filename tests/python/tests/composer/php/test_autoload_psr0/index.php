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

$g0 = new UniqueGlobalClass0();
$g0->init();

$g1 = new UniqueGlobalClass1();
$g1->init();

$t0 = new Test0();
$t0->echo();

$t1 = new foo\Test1();
$t1->echo();

$t2 = new foo\bar\Test2();
$t2->echo();

$t3 = new my_test3();
$t3->echo();

$t4 = new foo\my_test4();
$t4->echo();

$t5 = new foo\bar\my_test5();
$t5->echo();

echo "alright\n";
