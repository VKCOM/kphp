@ok
KPHP_ENABLE_MODULITE=1
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

require_once __DIR__ . '/plain/plain.php';

function globalDemo() {
    echo Utils003\Strings003::$name, "\n";
    Utils003\Strings003::hidden2();
    Utils003\Strings003::normal();
    plainPublic1();
    plainHidden1();
    plainHidden2();
    echo PLAIN_CONST_PUB, ' ', PLAIN_CONST_HID, "\n";
    echo Utils003\Impl003\Hasher003::calc1(), "\n";
}

globalDemo();
Feed003\Post003::demo();
Messages003\User003::demo();
Feed003\Post003::demoRequireUnexported();
