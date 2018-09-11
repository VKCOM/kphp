@ok
<?php

require_once "Classes/autoload.php";

$a = new \Classes\A;
\Classes\TemplateMagicStatic::print_magic($a);

$b = new \Classes\B;
\Classes\TemplateMagicStatic::print_magic($b);
