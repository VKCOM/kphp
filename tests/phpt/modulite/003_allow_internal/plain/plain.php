<?php

use Messages003\Infra003\Core003;   // ok, as not used

define('PLAIN_CONST_PUB_003', 1);
define('PLAIN_CONST_HID_003', 2);

function plainPublic1_003() {}
function plainPublic2_003() {}

function plainHidden1_003() { plainHidden2_003(); }
function plainHidden2_003() {
    (function() {
    /** @var $i ?\Utils003\Hidden003 */
    $i = null;
    })();
}

