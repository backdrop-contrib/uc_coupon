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
  $events['uc_coupon_auto_apply'] = array(
    'label' => t('Automatic coupons are being applied.'),
    'group' => t('Coupon'),
    'variables' => array(
      'cart_order' => array(
        'type' => 'uc_order',
        'label' => t('Cart Order'),
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
 * Implements hook_rules_action_info().
 */
function uc_coupon_rules_action_info() {
  $actions = array();
  
  // Apply an automatic coupon.
  $actions['uc_coupon_auto_apply'] = array(
    'label' =>  t('Apply an automatic coupon.'),
    'group' => t('Coupon'),
    'callbacks' => array( 'form_alter'=>'uc_coupon_auto_apply_form_alter'),
    'provides' => array(
      'coupon' => array(
        'type' => 'uc_coupon',
        'label' => t('Coupon'),
      ),
    ),
    'parameter' => array(
      'cid' => array(
        'type' => 'integer',
        'label' => t('Coupon'),
        'description' => t('Select the automatic coupon to apply.  The coupon will only be
        		applied if and when all of its restrictions are satisfied.'),
        'options list' => 'uc_coupon_auto_apply_options',
        'restriction' => 'input',
      ),
    ),
  );
  
  return $actions;
}

/**
 * Check if an order has a coupon applied.
 */
function uc_coupon_condition_order_has_coupon($order, $codes=array(), $check_current) {
  $check_codes = array();
  // We start with current valid codes, if specified
  if ($check_current) {
    $coupons = _uc_coupon_get_validated();
    foreach ($coupons as $coupon) {
      $check_codes[$coupon->code] = $coupon->code;
    }
  }
  // Now add any codes alreay in the order object (these take precedence).
  if (isset($order->data['coupons'])) {
    $check_codes = array_merge($check_codes, drupal_map_assoc(array_keys($order->data['coupons'])));
  }
  //dpm($check_codes,'here');
  if (count($check_codes) > 0) {
    $codes = array_filter($codes);
    // If no codes specified, match any coupon.
    if (count($codes)==0) {
      return TRUE;
    }

    // Check codes for exact or wildcard matches.
    foreach ($codes as $code) {
      foreach (array_keys($check_codes) as $check_code) {
        if (preg_match('/^' . str_replace('\*', '.*?', preg_quote(strtoupper(trim($code)), '/')) . '$/', $check_code)) {
          return TRUE;
        }
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


/**
 * Action callback to apply a coupon.
 * This simply adds the coupon to the $_SESSION['uc_coupons'] array.  
 * It will be validated when the cart is rebuilt.
 */
function uc_coupon_auto_apply($cid) {
  $coupon = uc_coupon_load($cid);
  if ($coupon->cid) {
    $_SESSION['uc_coupons'][$coupon->code] = 'auto';
    return array('coupon' => $coupon);
  }
  else {
    return array('coupon' => NULL);
  }
}

/**
 * Generate list of options for auto apply action.
 */
function uc_coupon_auto_apply_options() {
  $rows = db_query('SELECT cid, name, code FROM {uc_coupons} WHERE status = :status AND bulk = :bulk', 
    array(':status' => 1, ':bulk' => 0, ));
  $ops = array();
  foreach ($rows as $row) {
    $ops[$row->cid] = t("@name (Code: @code)", array('@name' => $row->name, '@code' => $row->code)); 
  }
  return $ops;
}

/**
 * Implementatino of 'form_alter' callback for the auto apply action.
 * Adds some help text.
 */
function uc_coupon_auto_apply_form_alter(&$form, $form_state) {
  $form['important_note'] = array(
    '#markup' => '<h4>' . t('Important Note:') . '</h4><p><strong>' . t('This action is intended to be used only with 
    the &ldquo;%event&rdquo; event. Using this in reaction to another event may have unpredictable results.',
    array('%event' => t('Automatic coupons are being applied'))) . '</strong></p><p>' .
    t('The check for automatic coupons is performed whenever the cart contents are updated.  At that time, if the
    conditions of this rule are met, the coupon specified below will be validated against the current cart contents,
    and applied if it passes validation.') . '</p>',
    '#weight' => -1
  );
}

