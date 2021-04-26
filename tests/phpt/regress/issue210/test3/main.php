@ok
<?php

require_once 'kphp_tester_include.php';

use Ast\BinaryExprTypes\BinaryPlus;
use Ast\Expr;

$node = new BinaryPlus(new Expr(), new Expr());
var_dump(get_class($node));
