<?php
/**
 * @file
 * Rules integration for uc_coupon
 */

/**
 * Implements hook_rules_data_info().
 */
function uc_coupon_rules_data_info() {
  $types['uc_coupon'] = array(
    'label' => t('Ubercart discount coupon'),
    'wrap' => FALSE,
    'group' => t('Ubercart'),
  );
}

/**
 * Implements hook_rules_event_info().
 * 
 * !TODO This event currently fires only when a valid coupon is applied via uc_coupon_submit().
 * Neet to revisit this, as there may be other circumstances in which a coupon is applied (e.g. via
 * the querystring).  Also there should be a matching event for a coupon being removed from an order.
 */
function uc_coupon_rules_event_info() {
  $events['uc_coupon_is_applied'] = array(
    'label' => t('A valid discount coupon is applied'),
    'group' => t('Coupon'),
    'variables' => array(
      'coupon' => array(
        'type' => 'uc_coupon',
        'label' => t('Coupon'),
      ),
    ),
  );
  return $events;
}

/**
 * Implements hook_rules_condition_info().
 */
function uc_coupon_rules_condition_info() {
  $conditions['uc_coupon_condition_order_has_coupon'] = array(
    'label' => t('Check if an order has a coupon applied'),
    'group' => t('Order'),
    'parameter' => array(
      'order' => array(
        'type' => 'uc_order',
        'label' => t('Order'),
        'optional' => TRUE,
        'default value' => NULL,
        'allow null' => TRUE
      ),
      'codes' => array(
        'type' => 'list<text>',
        'label' => t('Codes'),
        'description' => t('Enter coupon codes that this condition will apply to, one per line. Wildcards are allowed, e.g. COUPON* will match all codes beginning with COUPON. Leave blank to apply to any order with a coupon.'),
        'restriction' => 'input',
      ),
      'check_current' => array(
        'type' => 'boolean',
        'label' => t('Include currently applied coupon (if any).'),
        'description' => t('Check this box if this condition will apply only to orders that
    			are currently in checkout, in order to catch coupons applied before an order reaches
    			the review stage (e.g. for shipping quotes).  Leave unchecked if the condition will
    			apply to orders after checkout is complete (e.g. when payment notification is received 
    			or when order status is updated manually).'),
        'restriction' => 'input',
        'default value' => TRUE,
      ),
    ),
  );
  
  $conditions['uc_coupon_condition_is_bulk'] = array(
    'label' => t('Check if a coupon is a bulk coupon'),
    'group' => t('Coupon'),
    'parameter' => array(
      'coupon' => array(
        'type' => 'uc_coupon',
        'label' => t('Coupon'),
      ),
    ),
  );

  return $conditions;
}

/**
 * Check if an order has a coupon applied.
 */
function uc_coupon_condition_order_has_coupon($order, $codes=array(), $check_current) {
  if (isset($order->data['coupon'])) {
    $check_code = $order->data['coupon'];
  }
  elseif ($check_current && isset($_SESSION['uc_coupon'])) {
    $check_code = drupal_strtoupper(trim($_SESSION['uc_coupon']));
  }
  else {
    $check_code = FALSE;
  }
  $codes = array_filter($codes);
  if ($check_code) {
    // If no codes specified, match any coupon.
    if (count($codes)==0) {
      return TRUE;
    }

    // Check codes for exact or wildcard matches.
    foreach ($codes as $code) {
      if (preg_match('/^' . str_replace('\*', '.*?', preg_quote(strtoupper(trim($code)), '/')) . '$/', $check_code)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
 * Check that a coupon is a bulk coupon.
 */
function uc_coupon_condition_is_bulk($coupon) {
  return $coupon->bulk;
}
