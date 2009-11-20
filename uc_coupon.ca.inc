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
 * Check that the coupon is a bulk coupon.
 */
function uc_coupon_condition_is_bulk($coupon, $settings) {
  return $coupon->bulk;
}
