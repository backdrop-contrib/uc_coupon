<?php
// $Id$

/**
 * @file
 * Coupon order total field handler
 */
class uc_coupon_handler_field_all_orders_gross extends uc_coupon_handler_field_all_orders_total {
  function pre_render($values) {
    $this->pre_render_query('SUM(order_total+value)', $values);
    $this->field_alias = "uc_coupons_all_orders_total";
  }
}

