<?php

namespace Classes;

class User implements \ArrayAccess {
    private int $id;
    private string $name;
    /** @var int[] */
    private $friends;

    /**
     * @param $id_ int
     * @param $name_ string
     * @param $friends_ int[]
     */
    public function __construct($id_, $name_, $friends_) {
        $this->id = $id_;
        $this->name = $name_;
        $this->friends = $friends_;
    }

    /**
     * @param $offset mixed
     * @param $value  mixed
     */
    public function offsetSet($offset, $value) {
        if ($offset === null) {
            echo "Append is not supported";
            return;
        }
        $offset = strval($offset);

        var_dump("Offset in set " . $offset);


        if ($offset === "id") {
            $this->id = (int)$value;
            return;
        }
        if ($offset === "name") {
            $this->name = (string)$value;
            return;
        }
        if ($offset === "friends") {
            $this->friends = array_map('intval', $value);
            return;
        }
        echo "Unsupported offset \"" . $offset . "\"\n";
    }

    /**
     * @param $offset mixed
     * @return bool
     */
    public function offsetExists($offset) {
        $offset = strval($offset);
        return $offset === "id" || $offset === "name" || $offset === "friends";
    }

    /**
     * @param $offset mixed
     * @return void
     */
    public function offsetUnset($offset) {
        echo "Unset is not supported";
    }

    /**
     * @param $offset mixed
     * @return mixed
     */
    public function offsetGet($offset) {
        $offset = strval($offset);
        var_dump("Offset in get " . $offset);

        if ($offset === "id") {
            return $this->id;
        }
        if ($offset === "name") {
            return $this->name;
        }
        if ($offset === "friends") {
            return $this->friends;
        }
        echo "No match in get!!";

        return null;
    }
}
