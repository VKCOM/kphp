@ok
<?php
class A {
	protected const protectedConst = 'protectedConst';
	static function staticConstDump() {
		var_dump(self::protectedConst);
	}
	function constDump() {
		var_dump(self::protectedConst);
	}
}

class B {
	private const privateConst = 'privateConst';
	static function staticConstDump() {
		var_dump(self::privateConst);
	}
	function constDump() {
		var_dump(self::privateConst);
	}
}

A::staticConstDump();
(new A())->constDump();

B::staticConstDump();
(new B())->constDump();