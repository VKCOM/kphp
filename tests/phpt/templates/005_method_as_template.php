@ok
<?php

require_once 'polyfills.php';

$template_magic = new \Classes\TemplateMagic;
$a = new \Classes\A;
$template_magic->print_magic($a);

$template_magic->magic = 1;
$b = new \Classes\B;
$template_magic->print_magic($b);
