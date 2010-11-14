<?php
// $Id$

/**
 * Implementation of hook_ca_entity().
 */
function uc_coupon_ca_entity() {
  $entities['uc_coupon'] = array(
    '#title' => t('Ubercart coupon'),
    '#type' => 'object',
  );

  return $entities;
}

/**
 * Implementation of hook_ca_condition().
 */
function uc_coupon_ca_condition() {
  $conditions['uc_coupon_condition_order_has_coupon'] = array(
    '#title' => t('Check if an order has a coupon applied'),
    '#description' => t('Returns TRUE if the order had a coupon applied.'),
    '#category' => t('Order'),
    '#callback' => 'uc_coupon_condition_order_has_coupon',
    '#arguments' => array(
      'order' => array(
        '#entity' => 'uc_order',
      ),
    ),
  );
  $conditions['uc_coupon_condition_is_bulk'] = array(
    '#title' => t('Check if a coupon is a bulk coupon'),
    '#description' => t('Returns TRUE if the coupon is a bulk coupon.'),
    '#category' => t('Coupon'),
    '#callback' => 'uc_coupon_condition_is_bulk',
    '#arguments' => array(
      'coupon' => array(
        '#entity' => 'uc_coupon',
      ),
    ),
  );

  return $conditions;
}

/**
 * Check if an order has a coupon applied.
 */
function uc_coupon_condition_order_has_coupon($order, $settings) {
  return isset($order->data['coupon']);
}

/**
 * Check that the coupon is a bulk coupon.
 */
function uc_coupon_condition_is_bulk($coupon, $settings) {
  return $coupon->bulk;
}
