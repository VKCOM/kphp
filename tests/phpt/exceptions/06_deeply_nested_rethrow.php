@ok k2_skip
<?php

try {
	try {
		try {
			try {
				throw new Exception('KPHP');
			} catch (Exception $e) {
				var_dump($e->getMessage(), $e->getLine());
				throw $e;
			}
		} catch (Exception $e) {
			var_dump($e->getMessage(), $e->getLine());
			throw $e;
		}
	} catch (Exception $e) {
		var_dump($e->getMessage(), $e->getLine());
		throw $e;
	}
} catch (Exception $e) {
	var_dump($e->getMessage(), $e->getLine());
}
