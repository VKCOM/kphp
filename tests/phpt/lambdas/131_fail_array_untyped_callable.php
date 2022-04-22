@kphp_should_fail
/public \$arrCb = \[\];/
/function ao\(\$arg\)/
/Keywords 'callable' and 'object' have special treatment, they are not real types/
<?php

class CName {
    /** @var callable[] */
    public $arrCb = [];
}

new CName;

/**
 * @param tuple(object, int) $arg
 */
function ao($arg) {
}

ao(null);
