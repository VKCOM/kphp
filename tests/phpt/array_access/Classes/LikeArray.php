<?php

namespace Classes;

class LikeArray implements \ArrayAccess {
    /**  @var mixed[] */
    protected $data = [];

    /**
     * @param $offset mixed
     * @param $value  mixed
     */
    public function offsetSet($offset, $value) {
        if (is_null($offset)) {
            $this->data[] = $value;
        } else {
            $this->data[$offset] = $value;
        }
    }

    /**
     * @param $offset mixed
     * @return bool
     */
    public function offsetExists($offset) {
        return isset($this->data[$offset]);
    }

    /**
     * @param $offset mixed The offset to unset
     * @return void
     */
    public function offsetUnset($offset) {
        if ($this->offsetExists($offset)) {
            unset($this->data[$offset]);
        }
    }

    /**
     * @param $offset mixed
     * @return mixed
     */
    public function offsetGet($offset) {
        return $this->offsetExists($offset) ? $this->data[$offset] : null;
    }

    /**
     * @return mixed[]
     */
    public function keys() {
        return array_keys($this->data);
    }
}
