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
    private $a = self::privateConst;
    private $b = [self::privateConst];
	static function staticConstDump() {
		var_dump(self::privateConst);
	}
	function constDump() {
		var_dump(self::privateConst);
		var_dump($this->a, $this->b);
	}
}

A::staticConstDump();
(new A())->constDump();

B::staticConstDump();
(new B())->constDump();
