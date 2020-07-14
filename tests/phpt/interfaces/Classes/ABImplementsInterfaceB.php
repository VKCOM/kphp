<?php

namespace Classes;

class ABImplementsInterfaceB implements InterfaceBExtendsInterfaceA {
  /**
   * @return InterfaceA
   */
  function getSelf() {
    return $this;
  }

  /**
   * @return InterfaceBExtendsInterfaceA
   */
  function getSelfAsB() {
    return $this;
  }
}

