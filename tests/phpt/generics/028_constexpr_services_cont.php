@ok k2_skip
<?php

class ServicesContainer {
    private const HIDDEN = [
        ServicePrivate::class => true,
    ];

    private ?ServiceFoo $foo = null;
    private ?ServiceBar $bar = null;

    private int $fooArg;

    function __construct(int $fooArg) {
        $this->fooArg = $fooArg;
    }

    private function createFoo(): ServiceFoo {
        return new ServiceFoo($this->fooArg);
    }

    private function getBar(): ServiceBar {
        return $this->bar ?? ($this->bar = new ServiceBar('bar from ' . __CLASS__));
    }

    /**
     * Main API
     * @kphp-generic TService
     * @param class-string<TService> $serviceClass
     * @return TService
     */
    function getInstance(string $serviceClass) {
        switch ($serviceClass) {
        case ServiceFoo::class:
            return $this->foo ?? ($this->foo = $this->createFoo());
        case ServiceBar::class:
            return $this->getBar();
        case ServiceException::class:
            throw new Exception("ServiceException unimplemented");
        }

        if (isset(self::HIDDEN[$serviceClass])) {
            throw new Exception("$serviceClass is private and can not be used");
        }
        throw new Exception("$serviceClass not found");
    }
}

class ServiceFoo {
    private int $a;
    function __construct(int $a) { $this->a = $a; }
    function fooMethod() { echo "fooMethod $this->a\n"; }
}

class ServiceBar {
    private ?string $b;
    function __construct(?string $b) { $this->b = $b; }
    function barMethod() { echo "barMethod $this->b\n"; }
}

class ServicePrivate {
    function privateMethod() { echo "privateMethod\n"; }
}

class ServiceException {
    function exceptionMethod() { echo "exceptionMethod\n"; }
}

class ServiceUnknown {
    function unknownMethod() { echo "unknownMethod\n"; }
}

function demo1() {
    $factory = new ServicesContainer(42);
    $factory->getInstance(ServiceFoo::class)->fooMethod();
    $factory->getInstance(ServiceBar::class)->barMethod();
}

function demo2() {
    (new ServicesContainer(-1))->getInstance(ServicePrivate::class)->privateMethod();
}

function demo3() {
    (new ServicesContainer(-1))->getInstance(ServiceException::class)->exceptionMethod();
}

function demo4() {
    (new ServicesContainer(-1))->getInstance(ServiceUnknown::class)->unknownMethod();
}

function tryCatch(callable $cb) {
    try {
        $cb();
    } catch (Exception $ex) {
        echo "exception: {$ex->getMessage()}\n";
    }
}

tryCatch(fn() => demo1());
tryCatch(fn() => demo2());
tryCatch(fn() => demo3());
tryCatch(fn() => demo4());
