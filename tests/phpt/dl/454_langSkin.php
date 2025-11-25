@ok
<?php

function sLangBecomeTranslatorScript($langs){
return <<<EOF
(){
  Dropdown({$langs}, {
    width: 150,
  });
};
EOF;
}

function sLangAlBecomeTranslatorScript($langs){
return <<<EOF
{
  Dropdown({$langs}, {
    width: 150,
  });
};
EOF;
}


var_dump (sLangBecomeTranslatorScript(1));
var_dump (sLangAlBecomeTranslatorScript(1));

// var_dump (lcg_value());
var_dump (strlen(uniqid()));
var_dump (strlen(uniqid('aba')));
var_dump (strlen(uniqid('testtestsetsetsetsetseteest', false)));

var_dump (strlen(uniqid('', false)));
var_dump (strlen(uniqid('', false)));
var_dump (strlen(uniqid('', false)));
var_dump (strlen(uniqid('', false)));
var_dump (strlen(uniqid('', false)));
var_dump (strlen(uniqid('', true)));
var_dump (strlen(uniqid('', true)));
var_dump (strlen(uniqid('', true)));
var_dump (strlen(uniqid('', true)));
var_dump (strlen(uniqid('', true)));
var_dump (strlen(uniqid('my-own-prefix-', false)));
var_dump (strlen(uniqid('my-own-prefix-', false)));
var_dump (strlen(uniqid('my-own-prefix-', false)));
var_dump (strlen(uniqid('my-own-prefix-', false)));
var_dump (strlen(uniqid('my-own-prefix-', false)));
var_dump (strlen(uniqid('my-own-prefix-', true)));
var_dump (strlen(uniqid('my-own-prefix-', true)));
var_dump (strlen(uniqid('my-own-prefix-', true)));
var_dump (strlen(uniqid('my-own-prefix-', true)));
var_dump (strlen(uniqid('my-own-prefix-', true)));
