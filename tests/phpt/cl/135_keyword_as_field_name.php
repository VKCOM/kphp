@ok
<?php

require_once 'Classes/autoload.php';

$kafn = new Classes\KeywordAsFieldName();

var_dump($kafn->int);
var_dump($kafn->string);
var_dump($kafn->array);

var_dump(Classes\KeywordAsFieldName::try_);
var_dump(Classes\KeywordAsFieldName::switch_);
