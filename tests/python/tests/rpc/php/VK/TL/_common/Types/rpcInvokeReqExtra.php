<?php

/**
 * AUTOGENERATED, DO NOT EDIT! If you want to modify it, check tl schema.
 *
 * This autogenerated code represents tl class for typed RPC API.
 */

namespace VK\TL\_common\Types;

/**
 * @kphp-tl-class
 */
class rpcInvokeReqExtra {

  /** Field mask for $return_binlog_pos field */
  const BIT_RETURN_BINLOG_POS_0 = (1 << 0);

  /** Field mask for $return_binlog_time field */
  const BIT_RETURN_BINLOG_TIME_1 = (1 << 1);

  /** Field mask for $return_pid field */
  const BIT_RETURN_PID_2 = (1 << 2);

  /** Field mask for $return_request_sizes field */
  const BIT_RETURN_REQUEST_SIZES_3 = (1 << 3);

  /** Field mask for $return_failed_subqueries field */
  const BIT_RETURN_FAILED_SUBQUERIES_4 = (1 << 4);

  /** Field mask for $return_query_stats field */
  const BIT_RETURN_QUERY_STATS_6 = (1 << 6);

  /** Field mask for $no_result field */
  const BIT_NO_RESULT_7 = (1 << 7);

  /** Field mask for $wait_binlog_pos field */
  const BIT_WAIT_BINLOG_POS_16 = (1 << 16);

  /** Field mask for $string_forward_keys field */
  const BIT_STRING_FORWARD_KEYS_18 = (1 << 18);

  /** Field mask for $int_forward_keys field */
  const BIT_INT_FORWARD_KEYS_19 = (1 << 19);

  /** Field mask for $string_forward field */
  const BIT_STRING_FORWARD_20 = (1 << 20);

  /** Field mask for $int_forward field */
  const BIT_INT_FORWARD_21 = (1 << 21);

  /** Field mask for $custom_timeout_ms field */
  const BIT_CUSTOM_TIMEOUT_MS_23 = (1 << 23);

  /** Field mask for $supported_compression_version field */
  const BIT_SUPPORTED_COMPRESSION_VERSION_25 = (1 << 25);

  /** Field mask for $random_delay field */
  const BIT_RANDOM_DELAY_26 = (1 << 26);

  /** Field mask for $return_view_number field */
  const BIT_RETURN_VIEW_NUMBER_27 = (1 << 27);

  /** @var boolean */
  public $return_binlog_pos = false;

  /** @var boolean */
  public $return_binlog_time = false;

  /** @var boolean */
  public $return_pid = false;

  /** @var boolean */
  public $return_request_sizes = false;

  /** @var boolean */
  public $return_failed_subqueries = false;

  /** @var boolean */
  public $return_query_stats = false;

  /** @var boolean */
  public $no_result = false;

  /** @var int|null */
  public $wait_binlog_pos = null;

  /** @var string[]|null */
  public $string_forward_keys = null;

  /** @var int[]|null */
  public $int_forward_keys = null;

  /** @var string|null */
  public $string_forward = null;

  /** @var int|null */
  public $int_forward = null;

  /** @var int|null */
  public $custom_timeout_ms = null;

  /** @var int|null */
  public $supported_compression_version = null;

  /** @var float|null */
  public $random_delay = null;

  /** @var boolean */
  public $return_view_number = false;

  /**
   * @kphp-inline
   */
  public function __construct() {
  }

  /**
   * @return int
   */
  public function calculateFlags() {
    $mask = 0;

    if ($this->return_binlog_pos) {
      $mask |= self::BIT_RETURN_BINLOG_POS_0;
    }

    if ($this->return_binlog_time) {
      $mask |= self::BIT_RETURN_BINLOG_TIME_1;
    }

    if ($this->return_pid) {
      $mask |= self::BIT_RETURN_PID_2;
    }

    if ($this->return_request_sizes) {
      $mask |= self::BIT_RETURN_REQUEST_SIZES_3;
    }

    if ($this->return_failed_subqueries) {
      $mask |= self::BIT_RETURN_FAILED_SUBQUERIES_4;
    }

    if ($this->return_query_stats) {
      $mask |= self::BIT_RETURN_QUERY_STATS_6;
    }

    if ($this->no_result) {
      $mask |= self::BIT_NO_RESULT_7;
    }

    if ($this->wait_binlog_pos !== null) {
      $mask |= self::BIT_WAIT_BINLOG_POS_16;
    }

    if ($this->string_forward_keys !== null) {
      $mask |= self::BIT_STRING_FORWARD_KEYS_18;
    }

    if ($this->int_forward_keys !== null) {
      $mask |= self::BIT_INT_FORWARD_KEYS_19;
    }

    if ($this->string_forward !== null) {
      $mask |= self::BIT_STRING_FORWARD_20;
    }

    if ($this->int_forward !== null) {
      $mask |= self::BIT_INT_FORWARD_21;
    }

    if ($this->custom_timeout_ms !== null) {
      $mask |= self::BIT_CUSTOM_TIMEOUT_MS_23;
    }

    if ($this->supported_compression_version !== null) {
      $mask |= self::BIT_SUPPORTED_COMPRESSION_VERSION_25;
    }

    if ($this->random_delay !== null) {
      $mask |= self::BIT_RANDOM_DELAY_26;
    }

    if ($this->return_view_number) {
      $mask |= self::BIT_RETURN_VIEW_NUMBER_27;
    }

    return $mask;
  }

}
