@ok
<?php
interface CorrectId {
    public function __invoke(int $x) : bool;
}

class FilterClass {
    /**@var CorrectId|callable */
    public $correct_id = null;

    /**
     * @param CorrectId|callable $correct_id
     */
    public function set_correct_id($correct_id = null) : void {
        $this->correct_id = function($x) { return true; };
        $this->correct_id = $correct_id;
    }

    public function filter_arr(array $arr) : array {
        $res = [];
        $check_id = $this->correct_id;
        foreach ($arr as $value) {
            if ($check_id($value)) {
                $res[] = $value;
            }
        }

        return $res;
    }
}

function run() {
    $arr = [1, 2, 3, 4, 5, 6];

    $filter_class = new FilterClass();
    $filter_class->set_correct_id(function (int $x) : bool { return $x % 2 == 0; });
    var_dump($filter_class->filter_arr($arr));

    $filter_class->set_correct_id(function (int $x) : bool { return $x % 2 != 0; });
    var_dump($filter_class->filter_arr($arr));
}

run();
