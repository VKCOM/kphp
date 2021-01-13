@ok
<?php
require_once 'kphp_tester_include.php';

$r = (new Classes\UnifyNs\ConcreteUnify)->unify(new Classes\ResultNs\Result);

if ($r) {
    echo $r[0]->value, "\n";
} else {
    echo "false\n";
}

