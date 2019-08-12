@ok
<?php
require_once 'polyfills.php';
require_once 'Classes/autoload.php';

function demo(Classes\Z1 $z) {
    $z->num = 10;
    $z->setRawTuple($z->getInitial());
    $z->printAll();
}

demo(new Classes\Z1);
