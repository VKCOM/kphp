@ok
<?php
require_once 'kphp_tester_include.php';
require_once 'kphp_tester_include.php';

function demo(Classes\Z1 $z) {
    $z->num = 10;
    $z->setRawTuple($z->getInitial());
    $z->printAll();
}

demo(new Classes\Z1);
