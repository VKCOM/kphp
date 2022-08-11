<?php

use Messages003\Infra003\Core003;   // ok, as not used

define('PLAIN_CONST_PUB', 1);
define('PLAIN_CONST_HID', 2);

function plainPublic1() {}
function plainPublic2() {}

function plainHidden1() { plainHidden2(); }
function plainHidden2() {
    (function() {
    /** @var $i ?\Utils003\Hidden003 */
    $i = null;
    })();
}

