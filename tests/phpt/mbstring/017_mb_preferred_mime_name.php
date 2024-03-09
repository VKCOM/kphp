@ok
<?php

function test_preferred_mime_valid_name() {
	var_dump(mb_preferred_mime_name('sjis-win'));
	var_dump(mb_preferred_mime_name('SJIS'));
	var_dump(mb_preferred_mime_name('EUC-JP'));
	var_dump(mb_preferred_mime_name('UTF-8'));
	var_dump(mb_preferred_mime_name('ISO-2022-JP'));
	var_dump(mb_preferred_mime_name('JIS'));
	var_dump(mb_preferred_mime_name('ISO-8859-1'));
	var_dump(mb_preferred_mime_name('UCS2'));
	var_dump(mb_preferred_mime_name('UCS4'));
}

// function test_preferred_mime_invalid_name() {
// 	try {
// 		var_dump(mb_preferred_mime_name('BAD_NAME'));
// 	} catch (\ValueError $e) {
// 		echo $e->getMessage() . \PHP_EOL;
// 	}
// }