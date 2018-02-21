@ok
<?php
require_once 'Classes/autoload.php';

use Classes\A;

/**
 * @return A
 */
function getAInstance($initAVal = 198) {
  $a = new A();
  $a->a = $initAVal;
  return $a;
}

/**
 * @return A[]
 */
function getAArr() {
  $a1 = new A();
  $a1->a = 999;
  return array($a1, new A(), getAInstance(89));
}

$a = getAInstance();
$a->a = 126;
$a->printA();

$arr = getAArr();
$arr[0]->printA();
$arr[1]->printA();
$arr[2]->printA();

// regular phpdocs inside other constructions should not mess up parsing

$config = array();
$config += array(
    /** @deprecated */
    'group_moderators_manual' => array(),
);

function ff($a) {
    echo 1;
}

ff(/** @deprecated */ $config);

foreach(/** @deprecated */ [1,2] as /** @deprecated */ $i)
    ;
while( /** @deprecated */ 0 == 1);
for( /** @deprecated */ $i = 0; $i < 0; ++$i )
    ;

if( /** @deprecated */ true ) {
}

switch( /** @deprecated */ 3 ){
    /** @deprecated */
    case 1:
        /** @deprecated */
        break;
    /** @deprecated */
    default:
        ;
    /** @deprecated */
}

function sss() {
    return /** @lang JavaScript */
      <<<JAVASCRIPT
var abc = 1;
JAVASCRIPT;
}

$s = sss();

$arrr = [
    1,
    /** @type int*/ 2,
    3,
];
