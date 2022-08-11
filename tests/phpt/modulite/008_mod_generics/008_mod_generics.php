@ok
KPHP_ENABLE_MODULITE=1
KPHP_COMPOSER_ROOT={dir}
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

use Logic008\TestLogic008;

require_once __DIR__ . '/WithGen008/gen_f_008.php';

class B {
    function fb() { echo "fb\n"; }
}
class Either008_Bs {
    /** @var B */
    public $data;
    /** @var mixed */
    public $errors = [];
}

TestLogic008::testMapper();
test_either_creation();

$e1 = Either008::data(new Either008_Bs, new B);
TestLogic008::printEitherIsError('e1', $e1);
Either008::getData($e1)->fb();

$e2 = Either008::error/*<Either008_Bs, B>*/(new Either008_Bs, 'nulled', null);
TestLogic008::printEitherIsError('e2', $e2);
