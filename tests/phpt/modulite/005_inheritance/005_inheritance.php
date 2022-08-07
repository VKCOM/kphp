@ok
KPHP_ENABLE_MODULITE=1
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

use Feed005\Rank005\RankImpl1;
use Feed005\Rank005\RankImpl2;
use Feed005\SenderFactory;

GImportTrait::pubStaticFn();
$mmm = new GImportTrait;
$mmm->pubInstanceFn();
echo $mmm->i, ' ', GImportTrait::$count, "\n";

function callSend(Feed005\Send005\ISender $sender) {
    $sender->send();
}

RankImpl1::rank();
RankImpl2::rank();
RankImpl1::copyMe();
RankImpl2::copyMe();

callSend(SenderFactory::createSender('email'));
callSend(SenderFactory::createSender('sms'));

