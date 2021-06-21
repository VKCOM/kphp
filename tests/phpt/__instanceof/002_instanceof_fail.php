@kphp_should_fail
/left operand of 'instanceof' should be an instance of the class 'Classes\\WithBaseParent' or extends or implements it, but left operand has type 'Classes\\WithBaseParentAndIface'/
/left operand of 'instanceof' should be an instance of the class 'Classes\\Simple' or extends or implements it, but left operand has type 'Classes\\Base'/
/left operand of 'instanceof' should be an instance of the class 'Classes\\WithIBaseIface' or extends or implements it, but left operand has type 'Classes\\WithBaseParent'/
<?php
require_once 'kphp_tester_include.php';

$Base = new Classes\Base;
$Simple = new Classes\Simple;
$WithBaseParent = new Classes\WithBaseParent;
$WithIBaseIface = new Classes\WithIBaseIface;
$DeepExtendsBase = new Classes\DeepExtendsBase;
$DeepImplementsIBase = new Classes\DeepImplementsIBase;
$WithBaseParentAndIface = new Classes\WithBaseParentAndIface;


var_dump($WithBaseParentAndIface instanceof Classes\WithBaseParent);
var_dump($Base instanceof Classes\Simple);
var_dump($WithBaseParent instanceof Classes\WithIBaseIface);
