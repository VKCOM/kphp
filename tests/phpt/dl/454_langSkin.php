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

/*
var_dump (lcg_value());
var_dump (uniqid());
var_dump (uniqid('aba'));
var_dump (uniqid('testtestsetsetsetsetseteest', false));
var_dump (uniqid('', true));
*/
