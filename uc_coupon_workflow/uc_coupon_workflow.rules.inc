<?php
/**
 * @file
 * Rules integration for uc_coupon_workflow
 */

/**
 * Implements hook_rules_condition_info().
 */
function uc_coupon_workflow_rules_condition_info() {
  $conditions['uc_coupon_workflow_suspended'] = array(
    'label' => t('Check if an administrator has suspended automatic coupon workflow for this request.'),
    'group' => t('Coupon'),
  );
  return $conditions;
}


/**
 * Implements hook_rules_action_info().
 */
function uc_coupon_workflow_rules_action_info() {
  $actions = array();
  
  // Assign a coupon to a user.
  $actions['uc_coupon_workflow_assign'] = array(
    'label' =>  t('Assign a coupon to a user.'),
    'group' => t('Coupon'),
    'provides' => array(
      'coupon' => array(
        'type' => 'uc_coupon',
        'label' => t('Coupon'),
      ),
    ),
    'parameter' => array(
      'account' => array(
        'type' => 'user',
        'label' => t('Account'),    
        'description' => t('Select the user account to which this coupon should be assigned.'),
      ), 
      'cid' => array(
        'type' => 'integer',
        'label' => t('Coupon'),
        'description' => t('Select the base coupon to be assigned. 
        		The new user will be added to the user restrictions for the selected coupon, 
        		and the code will be sent to them by email.'),
        'options list' => 'uc_coupon_workflow_cid_options',
        'restriction' => 'input',
      ),
    ),
  );

  // Send an e-mail regarding a coupon.
  $actions['uc_coupon_workflow_email'] = array(
    'label' =>  t('Send an email regarding a coupon.'),
    'group' => t('Coupon'),
    'parameter' => array(
      'coupon' => array(
        'type' => 'uc_coupon',
        'label' => t('Coupon'),
      ),
      'from' => array(
        'type' => 'text',
        'label' => t('Sender'),
        'restriction' => 'input',
      ),
      'addresses' => array(
        'type' => 'text',
        'label' => t('Recipients'),
        'description' => t('Enter the email addresses to receive the notifications, one per line.'),
        'restriction' => 'input',
      ),
      'subject' => array(
        'type' => 'text',
        'label' => t('Subject'),
        'description' => t('Enter the subject line for the notifications.'),
        'restriction' => 'input',
      ),
      'message' => array(
        'type' => 'text',
        'label' => t('Message'),
        'description' => t('Enter the message body for the notifications.'),
        'restriction' => 'input',
      ),
      'format' => array(
        'type' => 'text',
        'label' => t('Message format'),
        'options list' => 'uc_coupon_purchase_message_formats',
      ),
    ),
  );
  
  return $actions;
}

/**
 * Options list callback for list of coupons to assign.
 */
function uc_coupon_workflow_cid_options() {
  return db_query('SELECT cid, name FROM {uc_coupons} WHERE status = 1')->fetchAllKeyed();
}

/**
 * Options list callback for message formats.
 */
function uc_coupon_workflow_message_formats() {
  global $user;

  $options = array();
  $formats = filter_formats($user);
  foreach ($formats as $format) {
    $options[$format->format] = $format->name;
  }
  return $options;
}


/**
 * For certain events (e.g. new user creation), administrators are provided
 * with an interface which allows them to prevent execution of coupon actions
 * based on those events.
 */
function uc_coupon_workflow_suspended($settings) {
  $auth = drupal_static('uc_coupon_workflow_suspended');
  return isset($auth) && $auth ? TRUE : FALSE; 
}


/**
 * Action callback to assign a coupon to a user.
 */
function uc_coupon_workflow_assign($account, $cid) {
  $coupon = uc_coupon_load($cid);
  if ($coupon->cid) {
    // Grant access to the coupon.
    $coupon->data['users'][$account->uid] = $account->uid;
    uc_coupon_save($coupon);
    return array('coupon' => $coupon);
  }
  else {
    return array('coupon' => NULL);
  }
}

/**
 * Action callback to send an e-mail regarding a coupon.
 */
function uc_coupon_workflow_email($coupon, $from, $addresses, $subject, $message, $format) {
  // Build the e-mail parameters.
  $params = array(
    'from' => $from,
    'subject' => $subject,
    'message' => $message,
    'format' => $format,
    'replacements' => array(), // The replacements are already handled by Rules.
  );
  
  // Split up our recipient e-mail addresses.
  $recipients = array();
  foreach (explode("\n", $addresses) as $address) {
    $recipients[] = trim($address);
  }

  // Send the e-mails.
  foreach ($recipients as  $email) {
    $sent = drupal_mail('uc_order', 'action-mail', $email, uc_store_mail_recipient_language($email), $params, $from);
    if (!$sent['result']) {
      watchdog('uc_coupon', 'Attempt to e-mail @email concerning coupon @cid failed.', array('@email' => $email, '@cid' => $coupon->cid), WATCHDOG_ERROR);
    }
  }
}

