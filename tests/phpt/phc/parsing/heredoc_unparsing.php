@ok
<?php
	// When unparsed, the HEREDOC strings should be output properly:
	//   * in-string syntax should be copied
	//   * terminating semi-colon (if any) should be on the same 
	//     line as the closing heredoc identifier

	$a = 1;

	echo <<<HTML
aa
HTML;

	echo <<<HTML
bb $a cc
HTML;

	echo <<<HTML
dd
HTML
	, <<<HTML
ee
HTML;
	
	echo <<<HTML
ff $a gg
HTML
	, <<<HTML
hh $a ii
HTML;

	echo "\n";
?>
