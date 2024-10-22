<?php

namespace Classes;

class LoggingLikeArray implements \ArrayAccess {
    /**  @var mixed[] */
    protected $data = [];

    /**
     * @param $offset mixed The offset to assign the value to
     * @param $value  mixed The value to set
     */
    public function offsetSet($offset, $value) {
        echo "offsetSet ";

        if (is_null($offset)) {
            echo "append\n";
            $this->data[] = $value;
        } else {
            echo "by index\n";
            $this->data[$offset] = $value;
        }
    }

    /**
     * @param $offset mixed
     * @return bool
     */
    public function offsetExists($offset) {
        echo "offsetExists\n";

        return isset($this->data[$offset]);
    }

    /**
     * @param $offset mixed The offset to unset
     * @return void
     */
    public function offsetUnset($offset) {
        echo "offsetUnset\n";

        if ($this->offsetExists($offset)) {
            unset($this->data[$offset]);
        }
    }

    /**
     * @param $offset mixed
     * @return mixed
     */
    public function offsetGet($offset) {
        echo "offsetGet\n";

        return $this->offsetExists($offset) ? $this->data[$offset] : null;
    }
}