@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

$callback = function ($x) { var_dump($x); };
$i = new Classes\ImplicitCapturingThis(100);
$i->pass_field_as_argument_of_lambda($callback);

$c = new Classes\ImplicitCapturingThis();
$c->pass_template_fields(function() { return new Classes\IntHolder(100); }, new Classes\AnotherIntHolder(90));


