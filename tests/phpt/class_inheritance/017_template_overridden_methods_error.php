@ok
<?php

interface SlicesVisitor {
    /**
     * @return any — T
     */
    function visitTitle(SliceTitle $slice);
}

interface Slice {
    /**
     * @kphp-generic T
     * @param T $visitor
     * @return T::_template
     */
    function accept($visitor);
}

abstract class AbstractSlice implements Slice {
    abstract function methodOfAbstractSlice();
}

class SliceTitle extends AbstractSlice {
    public static $next_id = 1;

    public int $id;

    function __construct() {
        $this->id = self::$next_id++;
    }

    function methodOfAbstractSlice() {
        echo __FUNCTION__, $this->id, "\n";
    }

    function methodOfSliceTitle() {
        echo __FUNCTION__, $this->id, "\n";
    }

    function accept($visitor) {
        return $visitor->visitTitle($this);
    }
}

class Visitor1 implements SlicesVisitor {
    protected SliceTitle $_template;

    function visitTitle(SliceTitle $slice): SliceTitle {
        echo get_class($this), get_class($slice), $slice->id, "\n";
        return $slice;
    }
}
class Visitor2 implements SlicesVisitor {
    protected SliceTitle $_template;

    function visitTitle(SliceTitle $slice): SliceTitle {
        echo get_class($this), get_class($slice), $slice->id, "\n";
        return $slice;
    }
}
class Visitor3 implements SlicesVisitor {
    protected SliceTitle $_template;

    function visitTitle(SliceTitle $slice): SliceTitle {
        echo get_class($this), get_class($slice), $slice->id, "\n";
        return $slice;
    }
}
class Visitor4 implements SlicesVisitor {
    protected SliceTitle $_template;

    function visitTitle(SliceTitle $slice): SliceTitle {
        echo get_class($this), get_class($slice), $slice->id, "\n";
        return $slice;
    }
}
class Visitor5 implements SlicesVisitor {
    protected SliceTitle $_template;

    function visitTitle(SliceTitle $slice): SliceTitle {
        echo get_class($this), get_class($slice), $slice->id, "\n";
        return $slice;
    }
}

(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();
(function(){ (new SliceTitle)->methodOfSliceTitle(); })();


/** @var Slice */
$slice = new SliceTitle;
$slice->accept(new Visitor1)->methodOfAbstractSlice();
$slice->accept(new Visitor1)->methodOfSliceTitle();
$slice->accept(new Visitor2)->methodOfAbstractSlice();
$slice->accept(new Visitor2)->methodOfSliceTitle();
$slice->accept(new Visitor3)->methodOfAbstractSlice();
$slice->accept(new Visitor3)->methodOfSliceTitle();

(function() {
    $slice_title = new SliceTitle;
    $slice_title->accept(new Visitor1)->methodOfAbstractSlice();
    $slice_title->accept(new Visitor1)->methodOfSliceTitle();
})();
(function() {
    $slice_title = new SliceTitle;
    $slice_title->accept(new Visitor2)->methodOfAbstractSlice();
    $slice_title->accept(new Visitor2)->methodOfSliceTitle();
})();
(function() {
    $slice_title = new SliceTitle;
    $slice_title->accept(new Visitor3)->methodOfAbstractSlice();
    $slice_title->accept(new Visitor3)->methodOfSliceTitle();
})();
(function() {
    $slice_title = new SliceTitle;
    $slice_title->accept(new Visitor4)->methodOfAbstractSlice();
    $slice_title->accept(new Visitor4)->methodOfSliceTitle();
})();
(function() {
    $slice_title = new SliceTitle;
    $slice_title->accept(new Visitor5)->methodOfAbstractSlice();
    $slice_title->accept(new Visitor5)->methodOfSliceTitle();
})();

// ------------

interface I {
    public function foo(callable $f);
}

class B implements I {
    public function foo(callable $f) {
        $f(20);
    }
}

class C implements I {
    public function foo(callable $f) {
        $f(60);
    }
}

class D implements I {
    public function foo(callable $f) {
        $f(123);
    }
}

$call_foo = function (I $b) {
    $b->foo(
        function($x) use ($b) {
            echo ("lambda1: called from " . get_class($b) . " with arg $x\n");
        }
    );
    $b->foo(
        function($x) use ($b) {
            echo ("lambda2: called from " . get_class($b) . " with arg $x\n");
        }
    );
};

$call_foo(new B());
$call_foo(new C());
$call_foo(new D());
