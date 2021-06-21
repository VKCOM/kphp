@ok
<?php
require_once 'kphp_tester_include.php';

$Base = new Classes\Base;
$Simple = new Classes\Simple;
$WithBaseParent = new Classes\WithBaseParent;
$WithIBaseIface = new Classes\WithIBaseIface;
$DeepExtendsBase = new Classes\DeepExtendsBase;
$DeepImplementsIBase = new Classes\DeepImplementsIBase;
$WithBaseParentAndIface = new Classes\WithBaseParentAndIface;

// simple
var_dump($Base instanceof Classes\Base);
var_dump($Simple instanceof Classes\Simple);
var_dump($WithBaseParent instanceof Classes\WithBaseParent);
var_dump($WithIBaseIface instanceof Classes\WithIBaseIface);
var_dump($DeepExtendsBase instanceof Classes\DeepExtendsBase);
var_dump($DeepImplementsIBase instanceof Classes\DeepImplementsIBase);
var_dump($WithBaseParentAndIface instanceof Classes\WithBaseParentAndIface);

// simple extends
var_dump($WithBaseParent instanceof Classes\Base);
var_dump($DeepExtendsBase instanceof Classes\Base);
var_dump($WithBaseParentAndIface instanceof Classes\Base);

// simple implements
var_dump($WithIBaseIface instanceof Classes\IBase);
var_dump($DeepImplementsIBase instanceof Classes\IBase);
var_dump($WithBaseParentAndIface instanceof Classes\IBase);

// deep extends
var_dump($DeepExtendsBase instanceof Classes\MiddleForBase);

// deep implements
var_dump($DeepImplementsIBase instanceof Classes\MiddleForIBase);
