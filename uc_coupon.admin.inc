<?php
/**
 * @file
 * Discount Coupons administration pages.
 */

/**
 * Store-wide coupon settings form.
 */
function uc_coupon_settings_form($form, &$form_state) {
  global $base_url;

  $form['uc_coupon_show_in_cart'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show coupons as an item in the cart.'),
    '#default_value' => variable_get('uc_coupon_show_in_cart', TRUE),
  );

  $form['uc_coupon_line_item_format'] = array(
    '#type' => 'textfield',
    '#title' => t('Line item title format'),
    '#description' => t('You may use [uc_coupon:name] and [uc_coupon:code] tokens.'),
    '#default_value' => variable_get('uc_coupon_line_item_format', t('Coupon !code', array('!code' => '[uc_coupon:code]'))),
  );

  $form['uc_coupon_default_usage'] = array(
    '#type' => 'radios',
    '#title' => t('Default redemption setting for new coupons'),
    '#options' => array(
      'single' => t('Single use per code.'),
      'multi' => t('Multiple uses per code.'),
    ),
    '#default_value' => variable_get('uc_coupon_default_usage', 'single'),
  );

  $form['uc_coupon_querystring'] = array(
    '#type' => 'textfield',
    '#title' => t('Query string parameter'),
    '#description' => t('If set, coupon codes can be applied by including them in any URL. Leave blank to disable this feature.<br />Example: if set to "coupon", visiting !base_url?coupon=CODE will apply the CODE coupon.', array('!base_url' => $base_url)),
    '#default_value' => variable_get('uc_coupon_querystring', ''),
    '#size' => 15,
  );

  return system_settings_form($form);
}

/**
 * Display a brief overview of system coupons
 *
 * @param $view_type
 *   'active' or 'inactive'
 */
function uc_coupon_display($view_type = 'active') {
  _uc_coupon_paypal_check();

  $header = array(
    array('data' => t('Actions')),
    array(
      'data' => t('Name'),
      'field' => 'name',
    ),
    array(
      'data' => t('Code'),
      'field' => 'code',
      'sort' => 'asc',
    ),
    array(
      'data' => t('Value'),
      'field' => 'value',
    ),
    array(
      'data' => t('Created'),
      'field' => 'created',
    ),
    array(
      'data' => t('Valid from'),
      'field' => 'valid_from',
    ),
    array(
      'data' => t('Valid until'),
      'field' => 'valid_until',
    ),
  );

  $query = db_select('uc_coupons', 'c')
    ->extend('TableSort')
    ->orderByHeader($header);
  $query->range(0, 20);
  $query->condition('c.status', $view_type == 'inactive' ? 0 : 1)->fields('c');
  
  $rows = array();
  $result = $query->execute();
  foreach ($result as $coupon) {
    $coupon->data = $coupon->data ? unserialize($coupon->data) : array();
    $rows[] = array(
      theme('uc_coupon_actions', array('coupon' => $coupon)),
      check_plain($coupon->name),
      check_plain($coupon->code) . ($coupon->bulk ? '* ' . t('(bulk)') : ''),
      uc_coupon_format_discount($coupon),
      format_date($coupon->created, 'custom', variable_get('uc_date_format_default', 'm/d/Y')),
      $coupon->valid_from ? format_date($coupon->valid_from, 'custom', variable_get('uc_date_format_default', 'm/d/Y')) : '-',
      $coupon->valid_until ? format_date($coupon->valid_until, 'custom', variable_get('uc_date_format_default', 'm/d/Y')) : '-',
    );
  }

  if (count($rows)) {
    $output = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('width' => '100%')));
    $output .= theme('pager', array('tags' => NULL));
  }
  else {
    switch ($view_type) {
      case 'active':
        $output = '<p>' . t('There are currently no active coupons in the system.') . '</p>';
        break;
      case 'inactive':
        $output = '<p>' . t('There are currently no inactive coupons in the system.') . '</p>';
        break;
    }
  }

  $output .= '<p>' . l(t('Add a new coupon.'), 'admin/store/coupons/add') . '</p>';

  return $output;
}

/**
 * View coupon details.
 *
 * @param $coupon object
 *   Coupon to view.
 */
function uc_coupon_view($coupon) {
  $rows = array();

  $rows[] = array(t('Name'), check_plain($coupon->name));

  if (!$coupon->status) {
    $status = t('Inactive');
  }
  elseif (!$coupon->valid_until) {
    $status = t('Active');
  }
  elseif (REQUEST_TIME < $coupon->valid_from) {
    $status = t('Not valid until @date', array('@date' => format_date($coupon->valid_from, 'custom', variable_get('uc_date_format_default', 'm/d/Y'))));
  }
  elseif (REQUEST_TIME > $coupon->valid_until) {
    $status = t('Expired on @date', array('@date' => format_date($coupon->valid_until, 'custom', variable_get('uc_date_format_default', 'm/d/Y'))));
  }
  else {
    $status = t('Active until @date', array('@date' => format_date($coupon->valid_until, 'custom', variable_get('uc_date_format_default', 'm/d/Y'))));
  }
  $rows[] = array(t('Status'), $status);

  if (!$coupon->bulk) {
    $rows[] = array(t('Code'), check_plain($coupon->code) . ' (' . l(t('Print'), 'admin/store/coupons/' . $coupon->cid . '/print') . ')');
    if ($coupon->max_uses) {
      $rows[] = array(t('Maximum uses'), $coupon->max_uses);
    }
  }
  else {
    $codes = '<strong>' . check_plain($coupon->code) . ' &times; ' . check_plain($coupon->data['bulk_number']) . '</strong>';
    $codes .= ' (' . l(t('Download codes'), 'admin/store/coupons/' . $coupon->cid . '/codes') . ')';
    $codes .= ' (' . l(t('Print all'), 'admin/store/coupons/' . $coupon->cid . '/print') . ')';
    for ($id = 0; $id < $coupon->data['bulk_number']; $id++) {
      $code = uc_coupon_get_bulk_code($coupon, $id);
      $codes .= '<br />' . check_plain($code) . ' (' . l(t('Print'), 'admin/store/coupons/' . $coupon->cid . '/print/' . $code) . ')';
    }

    $rows[] = array(t('Codes'), $codes);
    //$rows[] = array(t('Bulk seed'), check_plain($coupon->bulk_seed));
    if ($coupon->max_uses) {
      $rows[] = array(t('Maximum uses per code'), $coupon->max_uses);
    }
  }

  $rows[] = array(t('Discount value'), uc_coupon_format_discount($coupon));

  switch ($coupon->data['apply_to']) {
    case 'subtotal':
      $applies = t('Order subtotal');
      break;
    case 'products':
      $applies = t('Matching products');
      break;
    case 'cheapest':
      $applies = format_plural($coupon->data['apply_count'], 'Cheapest product', '@count cheapest products');
      break;
    case 'expensive':
      $applies = format_plural($coupon->data['apply_count'], 'Most expensive product', '@count most expensive products');
      break;
  }
  $rows[] = array(t('Applied to'), $applies);

  if ($coupon->data['apply_to'] != 'subtotal') {
    $restrict = array();
    if (isset($coupon->data['product_types'])) {
      $key = format_plural(count($coupon->data['product_types']), 'All products in class', 'All products in classes');
      $restrict[$key] = $coupon->data['product_types'];
    }
    if (isset($coupon->data['products'])) {
      $products = array();
      foreach ($coupon->data['products'] as $nid) {
        $products[] = check_plain(db_query("SELECT title FROM {node} WHERE nid = :nid", array(':nid' => $nid))->fetchField());
      }
      if (isset($coupon->data['negate_products'])) {
        $restrict[t('All products except')] = $products;
      }
      else {
        $restrict[format_plural(count($products), 'Product', 'Products')] = $products;
      }
    }
    if (isset($coupon->data['skus'])) {
      $restrict[format_plural(count($coupon->data['skus']), 'SKU', 'SKUs')] = $coupon->data['skus'];
    }
    if (isset($coupon->data['terms'])) {
      $terms = array();
      foreach ($coupon->data['terms'] as $tid) {
        $terms[] = check_plain(db_query("SELECT name FROM {taxonomy_term_data} WHERE tid = :tid", array(':tid' => $tid))->fetchField());
      }
      if (isset($coupon->data['negate_terms'])) {
        $restrict[t('All taxonomy terms except')] = $terms;
      }
      else {
        $restrict[format_plural(count($terms), 'Taxonomy term', 'Taxonomy terms')] = $terms;
      }
    }
    if ($restrict) {
      $or = FALSE;
      foreach ($restrict as $title => &$restriction) {
        if ($or) {
          $title = t('or') . ' ' . $title;
        }
        $restriction = $title . ': <em>' . implode('</em>, <em>', $restriction) . '</em>';
        $or = TRUE;
      }
      $rows[] = array(t('Product restrictions'), implode($restrict, '<br />'));
    }
  }

  $restrict = array();
  if (isset($coupon->data['users'])) {
    $users = array();
    foreach ($coupon->data['users'] as $uid) {
      $users[] = check_plain(db_query("SELECT name FROM {users} WHERE uid = :uid", array(':uid' => $uid))->fetchField());
    }
    $restrict['Users'] = $users;
  }
  if (isset($coupon->data['max_uses_per_user'])) {
    $restrict['Maximum uses per user'] = array($coupon->data['max_uses_per_user']);
  }
  if (isset($coupon->data['roles'])) {
    $restrict['Roles'] = $coupon->data['roles'];
  }
  if ($coupon->data['wholesale'] == 2) {
    $restrict['Has permission'] = array('coupon wholesale pricing');
  }
  elseif ($coupon->data['wholesale'] == 3) {
    $restrict['Does not have permission'] = array('coupon wholesale pricing');
  }
  if ($restrict) {
    foreach ($restrict as $title => &$restriction) {
      $restriction = $title . ': <em>' . implode('</em>, <em>', $restriction) . '</em>';
    }
    $rows[] = array(t('User restrictions'), implode($restrict, '<br />'));
  }

  if ($coupon->minimum_order > 0) {
    $rows[] = array(t('Order restrictions'), t('Minimum subtotal') . ': <em>' . uc_currency_format($coupon->minimum_order) . '</em>');
  }

  foreach ($rows as &$row) {
    $row[0] = array(
      'header' => TRUE,
      'data' => $row[0],
    );
  }

  $output = theme('table', array('header' => array(), 'rows' => $rows));
  return $output;
}

/**
 * Print a coupon, or set of bulk coupons.
 */
function uc_coupon_print($coupon, $code = NULL, $op = 'view') {
  if ($code) {
    $codes = array($code);
  }
  elseif (!$coupon->bulk) {
    $codes = array($coupon->code);
  }
  else {
    $codes = array();
    for ($id = 0; $id < $coupon->data['bulk_number']; $id++) {
      $codes[] = uc_coupon_get_bulk_code($coupon, $id);
    }
  }

  $output = '';
  foreach ($codes as $code) {
    $output .= theme('uc_coupon_certificate', array('coupon' => $coupon, 'code' => $code));
  }

  drupal_add_css(drupal_get_path('module', 'uc_coupon') . '/uc_coupon.css');

  if ($op == 'print') {
    echo theme('uc_coupon_page', array('content' => $output));
    exit;
  }

  return $output;
}

/**
 * Coupon add/edit form.
 *
 * @param $coupon object
 *   Coupon object, or NULL to add a new coupon.
 */
function uc_coupon_add_form($form, &$form_state, $coupon = NULL) {
  _uc_coupon_paypal_check();

  drupal_add_css(drupal_get_path('module', 'uc_coupon') . '/uc_coupon.css');
  drupal_add_js(drupal_get_path('module', 'uc_coupon') . '/uc_coupon.admin.js');

  if ($coupon) {
    $value = $coupon;
    $used = db_query("SELECT COUNT(*) FROM {uc_coupons_orders} WHERE cid = :cid", array(':cid' => $value->cid))->fetchField();
    $form['#uc_coupon_cid'] = $value->cid;
    $form['#uc_coupon'] = $value;
    $form['#uc_coupon_used'] = $used;
    $use_validity = $value->valid_until;
  }
  else {
    $use_validity = FALSE;
    $value->minimum_order = 0;
    $value->data['apply_to'] = 'subtotal';
    $value->max_uses = variable_get('uc_coupon_default_usage', 'single') == 'single' ? 1 : 0;
    $used = 0;
  }

  if (!$use_validity) {
    $value->valid_from = REQUEST_TIME;
    $value->valid_until = REQUEST_TIME;
  }

  // Handle AJAX functionality.
  $ajax_fields = array(
    'products' => 'admin/store/coupons/autocomplete/node',
    'skus' => '',
    'terms' => 'admin/store/coupons/autocomplete/term',
    'users' => 'admin/store/coupons/autocomplete/user',
    'roles' => 'admin/store/coupons/autocomplete/role',
  );
  
  if (empty($form_state['ajax_submission'])) {
    // Set the initial field count.
    foreach (array_keys($ajax_fields) as $field) {
      $form_state['storage']['field_count'][$field] = isset($value->data[$field]) ? (count($value->data[$field]) + 3) : 3;
    }
  }
  else {
    // Use data from the AJAX submission.
    $value->data = $form_state['coupon_data'];

    // Increase the count for the selected field.
    $form_state['storage']['field_count'][$form_state['ajax_submission']] += 3;

    // Reset form action, otherwise validation errors go wrong.
    $form['#action'] = $form_state['action'];
  }

  foreach (array_keys($ajax_fields) as $field) {
    if (!isset($value->data[$field])) {
      $value->data[$field] = array();
    }
  }

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Coupon name'),
    '#default_value' => isset($value->name) ? $value->name : '',
    '#required' => TRUE,
    '#weight' => -15,
  );

  $form['code'] = array(
    '#type' => 'textfield',
    '#title' => t('Coupon code'),
    '#description' => t('Coupon codes cannot be changed once they have been used in an order.'),
    '#default_value' => isset($value->code) ? $value->code : '',
    '#size' => 25,
    '#required' => !$used,
    '#maxlength' => 50,
    '#disabled' => $used,
    '#weight' => -10,
  );

  $form['bulk'] = array(
    '#type' => 'fieldset',
    '#title' => t('Bulk coupon codes'),
    '#description' => t('The coupon code entered above will be used to prefix each generated code.'),
    '#collapsible' => TRUE,
    '#collapsed' => isset($value->bulk) ? !$value->bulk : TRUE,
    '#weight' => -5,
  );

  if (!$used) {
    $form['bulk']['bulk_generate'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable bulk generation of coupon codes.'),
      '#default_value' => isset($value->bulk) ? $value->bulk : FALSE,
    );
  }
  else {
    $form['bulk']['bulk_generate'] = array(
      '#type' => 'value',
      '#default_value' => $value->bulk,
    );
  }

  $form['bulk']['bulk_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of codes to generate'),
    '#default_value' => isset($value->data['bulk_number']) ? $value->data['bulk_number'] : NULL,
    '#size' => 10,
    '#maxlength' => 10,
    '#disabled' => $used,
  );

  $form['bulk']['bulk_length'] = array(
    '#type' => 'select',
    '#title' => t('Code length'),
    '#description' => t('The number of characters selected here will be appended to the code entered above when generating bulk coupons.'),
    '#default_value' => isset($value->data['bulk_length']) ? $value->data['bulk_length'] : NULL,
    '#options' => drupal_map_assoc(range(8, 30)),
    '#disabled' => $used,
  );

  $form['status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Coupon is active.'),
    '#description' => t('Only active coupons can be used at checkout.'),
    '#default_value' => isset($value->status) ? $value->status : FALSE,
  );

  $form['discount'] = array(
    '#type' => 'textfield',
    '#title' => t('Discount'),
    '#default_value' => isset($coupon->value) ? uc_coupon_format_discount($value, FALSE) : '',
    '#size' => 6,
    '#description' => t('Three discount types are possible:') . theme('item_list', array('items' => array(
        t('Fixed price discount; enter "15" for $15.00 off.'),
        t('Percentage discount; enter "15%" for 15% off.'),
        t('Set product price; enter "=15" to make all matching products $15.00 after discount.'),
      ))),
    '#field_prefix' => variable_get('uc_sign_after_amount', FALSE) ? '' : variable_get('uc_currency_sign', '$'),
    '#field_suffix' => variable_get('uc_sign_after_amount', FALSE) ? variable_get('uc_currency_sign', '$') : '',
    '#required' => TRUE,
  );

  $form['apply_to'] = array(
    '#type' => 'radios',
    '#title' => t('Apply discount to'),
    '#options' => array(
      'subtotal' => t('Order subtotal'),
      'products' => t('All matching product(s)'),
      'cheapest' => t('The cheapest matching product(s)'),
      'expensive' => t('The most expensive matching product(s)'),
    ),
    '#description' => t('Coupons with no restrictions will match all products.'),
    '#default_value' => $value->data['apply_to'],
  );
  $form['apply_count'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of products to discount'),
    '#default_value' => isset($value->data['apply_count']) ? $value->data['apply_count'] : '',
    '#size' => 5,
  );

  $form['use_validity'] = array(
    '#type' => 'checkbox',
    '#title' => t('Restrict coupon by date.'),
    '#default_value' => $use_validity,
  );
  $form['valid_from'] = array(
    '#type' => 'date',
    '#title' => t('Start date'),
    '#default_value' => array(
      'year' => format_date($value->valid_from, 'custom', 'Y'),
      'month' => format_date($value->valid_from, 'custom', 'n'),
      'day' => format_date($value->valid_from, 'custom', 'j'),
    ),
    '#after_build' => array('_uc_coupon_date_range'),
  );
  $form['valid_until'] = array(
    '#type' => 'date',
    '#title' => t('Expiry date'),
    '#default_value' => array(
      'year' => format_date($value->valid_until, 'custom', 'Y'),
      'month' => format_date($value->valid_until, 'custom', 'n'),
      'day' => format_date($value->valid_until, 'custom', 'j'),
    ),
    '#after_build' => array('_uc_coupon_date_range'),
  );

  $form['max_uses'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of redemptions (per code)'),
    '#default_value' => $value->max_uses ? $value->max_uses : '',
    '#description' => t('If set, the coupon can only be used this number of times. For bulk coupons, this applies to each available code.'),
    '#size' => 5,
  );
  $form['max_uses_per_user'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum number of redemptions (per user)'),
    '#default_value' => isset($value->data['max_uses_per_user']) ? $value->data['max_uses_per_user'] : '',
    '#description' => t('If set, each user can only use the coupon this number of times.'),
    '#size' => 5,
  );

  $form['order'] = array(
    '#type' => 'fieldset',
    '#title' => t('Minimum order limits'),
    '#collapsible' => TRUE,
    '#collapsed' => $value->minimum_order < 0.01 && !isset($value->data['minimum_qty']),
  );
  $form['order']['minimum_order'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum order total'),
    '#default_value' => $value->minimum_order < 0.01 ? '' : $value->minimum_order,
    '#size' => 6,
    '#description' => t('If set, the subtotal of products must be at least this amount for the coupon to be accepted.'),
    '#field_prefix' => variable_get('uc_sign_after_amount', FALSE) ? '' : variable_get('uc_currency_sign', '$'),
    '#field_suffix' => variable_get('uc_sign_after_amount', FALSE) ? variable_get('uc_currency_sign', '$') : '',
  );
  $form['order']['minimum_qty'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum order quantity'),
    '#default_value' => isset($value->data['minimum_qty']) ? $value->data['minimum_qty'] : '',
    '#size' => 5,
    '#description' => t('If set, at least this many products must be in the cart for the coupon to be accepted.'),
  );
  $form['order']['minimum_qty_restrict'] = array(
    '#type' => 'checkbox',
    '#title' => t('Only count applicable products'),
    '#default_value' => isset($value->data['minimum_qty_restrict']) ? TRUE : FALSE,
    '#description' => t('If checked, only applicable products as configured below will count towards the minimum quantity.'),
  );


  $form['product_types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Applicable product classes'),
    '#description' => t('Selecting a class will apply this coupon to <strong>all</strong> products of the class, overriding other restrictions except for products or terms explicitly excluded below.'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($value->data['product_types']),
  );
  $form['product_types']['product_types'] = array(
    '#type' => 'checkboxes',
    '#options' => array_map('check_plain', array_intersect_key(node_type_get_names(), drupal_map_assoc(uc_product_types()))),
    '#default_value' => isset($value->data['product_types']) ? $value->data['product_types'] : array(),
  );

  $form['products'] = array(
    '#type' => 'fieldset',
    '#title' => t('Applicable products'),
    '#description' => t('Enter one or more products below to restrict this coupon to a set of products, regardless of any product attributes. Discounts will apply to each matching product.'),
  );
  $form['products']['negate_products'] = array(
    '#type' => 'radios',
    '#default_value' => isset($value->data['negate_products']) ? 1 : 0,
    '#options' => array(
      0 => t('Apply coupon to products listed below.'),
      1 => t('Apply coupon to all products except those listed below.'),
    ),
    '#tree' => FALSE,
  );
  foreach ($value->data['products'] as $nid) {
    $title = db_query('SELECT title FROM {node} WHERE nid = :nid', array(':nid' => $nid))->fetchField();
    $form['products'][] = array(
      '#type' => 'textfield',
      '#default_value' => $title . ' [nid:' . $nid . ']',
      '#autocomplete_path' => 'admin/store/coupons/autocomplete/node',
      '#maxlength' => 255,
    );
  }

  $form['skus'] = array(
    '#type' => 'fieldset',
    '#title' => t('Applicable SKUs'),
    '#description' => t('Enter one or more SKUs below to restrict this coupon to a set of SKUs, allowing coupons to apply to specific products or attribute options. Discounts will apply to matching SKUs. Wildcards are supported, e.g. "E*" will match all products with SKUs beginning with E.'),
  );
  foreach ($value->data['skus'] as $sku) {
    $form['skus'][] = array(
      '#type' => 'textfield',
      '#default_value' => $sku,
    );
  }

  $form['terms'] = array(
    '#type' => 'fieldset',
    '#title' => t('Applicable taxonomy terms'),
    '#description' => t('Enter one or more taxonomy terms (categories) below to restrict this coupon to a set of products. Discounts will apply to all matching products with these terms.'),
  );
  $form['terms']['negate_terms'] = array(
    '#type' => 'radios',
    '#default_value' => isset($value->data['negate_terms']) ? 1 : 0,
    '#options' => array(
      0 => t('Apply coupon to products with terms listed below.'),
      1 => t('Apply coupon to all products except those with terms listed below.'),
    ),
    '#tree' => FALSE,
  );
  foreach ($value->data['terms'] as $tid) {
    $name = db_query('SELECT name FROM {taxonomy_term_data} WHERE tid = :tid', array(':tid' => $tid))->fetchField();
    $form['terms'][] = array(
      '#type' => 'textfield',
      '#default_value' => $name . ' [tid:' . $tid . ']',
      '#autocomplete_path' => 'admin/store/coupons/autocomplete/term',
    );
  }

  $form['users'] = array(
    '#type' => 'fieldset',
    '#title' => t('User restrictions'),
    '#description' => t('Enter one or more user names and/or "anonymous users" below to make this coupon valid only for those users.'),
  );
  foreach ($value->data['users'] as $uid) {
    $username = $uid ? db_query('SELECT name FROM {users} WHERE uid = :uid', array(':uid' => $uid))->fetchField() : t('anonymous users');
    $form['users'][] = array(
      '#type' => 'textfield',
      '#default_value' => $username . ' [uid:' . $uid . ']',
      '#autocomplete_path' => 'admin/store/coupons/autocomplete/user',
    );
  }

  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Role restrictions'),
    '#description' => t('Enter one or more role names below to make this coupon valid only for users with those roles.'),
  );
  foreach ($value->data['roles'] as $role) {
    $form['roles'][] = array(
      '#type' => 'textfield',
      '#default_value' => $role,
      '#autocomplete_path' => 'admin/store/coupons/autocomplete/role',
    );
  }

  // Add common settings and ajax functionality to restriction fieldsets.
  foreach ($ajax_fields as $field => $path) {
    $form[$field] = array_merge($form[$field], array(
      '#tree' => TRUE,
      '#collapsible' => TRUE,
      '#collapsed' => empty($value->data[$field]) && empty($form_state['ajax_submission']),
      '#prefix' => '<div id="ajax-' . $field . '">',
      '#suffix' => '</div>',
      'more_' . $field => array(
        '#type' => 'submit',
        '#value' => t('Add more items'),
        '#weight' => 1,
        '#name' =>  'op_' . $field,
        '#limit_validation_errors' => array(),
        '#submit' => array('uc_coupon_ajax_submit'),
        '#ajax' => array( 
          'callback' => 'uc_coupon_ajax_callback',
          'wrapper' => 'ajax-' . $field,
          'effect' => 'fade',
        ),
        '#tree' => FALSE,
      ),
    ));

    // Add blank fields if needed.
    for ($i = count($value->data[$field]); $i < $form_state['storage']['field_count'][$field]; $i++) {
      $form[$field][] = array(
        '#type' => 'textfield',
        '#autocomplete_path' => $path ? $path : NULL,
      );
    }
  }

  $form['deprecated'] = array(
    '#type' => 'fieldset',
    '#title' => t('Deprecated options'),
    '#description' => t('These options will be removed in a future version.'),
    '#collapsible' => TRUE,
    '#collapsed' => !isset($value->data['wholesale']) || $value->data['wholesale'] == 1,
  );
  $form['deprecated']['wholesale'] = array(
    '#type' => 'radios',
    '#title' => t('Wholesale permissions'),
    '#description' => t('Select the groups who are able to use this coupon. It is recommended that you leave this option as "Both wholesale and retail" use the role selection above instead.'),
    '#default_value' => isset($value->data['wholesale']) ? $value->data['wholesale'] : 1,
    '#options' => array(
      '1' => t('Both wholesale and retail'),
      '2' => t('Wholesale buyers only'),
      '3' => t('Retail buyers only'),
    ),
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save coupon'),
    '#weight' => 10,
  );

  return $form;
}

function _uc_coupon_date_range($form_element) {
  $form_element['year']['#options'] = drupal_map_assoc(range(2008, 2020));
  return $form_element;
}

/**
 * Ajax form element selector callback for "Add more items" buttons in restrictions fieldsets.
 */
function uc_coupon_ajax_callback($form, $form_state) {
  return $form[$form_state['ajax_submission']];
}

/**
 * Ajax submit handler for "Add more items" button in restrictions fieldsets.
 */
function uc_coupon_ajax_submit($form, &$form_state) {
  $elem = $form_state['triggering_element']['#name'];
  if (substr($elem, 0, 3) === 'op_') {
    $form_statet['rebuild'] = TRUE;
    $form_state['ajax_submission'] = substr($elem, 3);
    uc_coupon_add_form_submit($form, $form_state);
  }
}

/**
 * Autocomplete callback for product restrictions.
 */
function uc_coupon_autocomplete_node($string) {
  $matches = array();
  $product_types = uc_product_types();

  $result = db_query("SELECT nid, title FROM {node} WHERE type IN (:types) AND title LIKE :title", array( ':types' => $product_types, ':title' => "%$string%"));
  foreach ($result as $row) {
    $title = check_plain($row->title);
    $matches[$title . ' [nid:' . $row->nid . ']'] = $title;
  }
  print drupal_json_encode($matches);
  exit;
}

/**
 * Autocomplete callback for term restrictions.
 */
function uc_coupon_autocomplete_term($string) {
  $matches = array();
  $result = db_query("SELECT tid, name FROM {taxonomy_term_data} WHERE name LIKE ':name'", array(':name' => "%$string%"));
  foreach ($result as $row) {
    $matches[$row->name . ' [tid:' . $row->tid . ']'] = $row->name;
  }
  print drupal_json_encode($matches);
  exit;
}

/**
 * Autocomplete callback for user restrictions.
 */
function uc_coupon_autocomplete_user($string) {
  $matches = array();
  $anonymous = t('anonymous users');
  if (strpos($anonymous, $string) !== FALSE) {
    $matches[$anonymous . ' [uid:0]'] = $anonymous;
  }

  $result = db_query("SELECT uid, name FROM {users} WHERE name LIKE :name", array( ':name' => "%$string%"));
  foreach ($result as $row) {
    $matches[$row->name . ' [uid:' . $row->uid . ']'] = $row->name;
  }
  print drupal_json_encode($matches);
  exit;
}

/**
 * Autocomplete callback for role restrictions.
 */
function uc_coupon_autocomplete_role($string) {
  $matches = array();
  $result = db_query("SELECT name FROM {role} WHERE name LIKE :name", array(':name' => "%$string%"));
  foreach ($result as $row) {
    $matches[$row->name] = $row->name;
  }
  print drupal_json_encode($matches);
  exit;
}

/**
 * Coupon form validate handler.
 */
function uc_coupon_add_form_validate($form, &$form_state) {
  drupal_add_css(drupal_get_path('module', 'uc_coupon') . '/uc_coupon.css');
  drupal_add_js(drupal_get_path('module', 'uc_coupon') . '/uc_coupon.admin.js');

  $name = db_query("SELECT name FROM {uc_coupons} WHERE code = :code AND cid <> :cid", 
    array(':code' => strtoupper($form_state['values']['code']), ':cid' => isset($form['#uc_coupon_cid']) ? $form['#uc_coupon_cid'] : NULL))->fetchField();
  if ($name) {
    form_set_error('code', t('Coupon code already used by %name.', array('%name' => $name)));
  }

  if (!preg_match('/^(=(?!.*%))?\d+(\.\d+)?%?$/', $form_state['values']['discount'])) {
    form_set_error('discount', t('Invalid discount.'));
  }

  if (substr($form_state['values']['discount'], 0, 1) == '=' && $form_state['values']['apply_to'] == 'subtotal') {
    form_set_error('apply_to', t('Set prices cannot be applied to the order subtotal.'));
  }

  if ($form_state['values']['apply_to'] == 'cheapest' || $form_state['values']['apply_to'] == 'expensive') {
    if (!preg_match('/^[1-9]\d*$/', $form_state['values']['apply_count'])) {
      form_set_error('apply_count', t('You must specify the maximum number of products to discount.'));
    }
  }

  foreach ($form_state['values']['products'] as $key => $product) {
    if ($product && !preg_match('/\[nid:(\d+)\]$/', $product)) {
      form_set_error('products][' . $key, t('Products must include the node ID.'));
    }
  }

  foreach ($form_state['values']['users'] as $key => $user) {
    if ($user && !preg_match('/\[uid:(\d+)\]$/', $user)) {
      form_set_error('users][' . $key, t('User names must include the user ID.'));
    }
  }

  if (!$form['#uc_coupon_used'] && $form_state['values']['bulk_generate'] && intval($form_state['values']['bulk_number']) <= 0) {
    form_set_error('bulk_number', t('You must specify the number of codes to generate.'));
  }

  if ($form_state['values']['use_validity']) {
    $valid_from = gmmktime(0, 0, 0, $form_state['values']['valid_from']['month'], $form_state['values']['valid_from']['day'], $form_state['values']['valid_from']['year']);
    $valid_until = gmmktime(0, 0, 0, $form_state['values']['valid_until']['month'], $form_state['values']['valid_until']['day'], $form_state['values']['valid_until']['year']);
    if ($valid_from > $valid_until) {
      form_set_error('valid_from', t('The coupon start date must be before the expiration date.'));
    }
  }
}

/**
 * Coupon form submit handler.
 */
function uc_coupon_add_form_submit($form, &$form_state) {
  // If the coupon was previously used, reset disabled textfields to their original values.
  if ($form['#uc_coupon_used']) {
    $form_state['values']['code'] = $form['#uc_coupon']->code;
    $form_state['values']['bulk_number'] = $form['#uc_coupon']->data['bulk_number'];
  }

  // Ensure all fields of original coupon object propagate to presave hook.
  $coupon = (isset($form['#uc_coupon'])) ? $form['#uc_coupon'] : new stdClass;
  if (isset($form['#uc_coupon_cid'])) {
    $coupon->cid = $form['#uc_coupon_cid'];
  }

  // Set basic coupon information.
  $coupon->name = $form_state['values']['name'];
  $coupon->code = strtoupper($form_state['values']['code']);
  $coupon->bulk = $form_state['values']['bulk_generate'];
  $coupon->data['bulk_number'] = $form_state['values']['bulk_generate'] ? $form_state['values']['bulk_number'] : 0;
  $coupon->data['bulk_length'] = $form_state['values']['bulk_length'];
  if ($form_state['values']['use_validity']) {
    $coupon->valid_from = gmmktime(0, 0, 0, $form_state['values']['valid_from']['month'], $form_state['values']['valid_from']['day'], $form_state['values']['valid_from']['year']);
    $coupon->valid_until = gmmktime(0, 0, 0, $form_state['values']['valid_until']['month'], $form_state['values']['valid_until']['day'], $form_state['values']['valid_until']['year']);
  }
  else {
    $coupon->valid_from = $coupon->valid_until = 0;
  }
  $coupon->status = $form_state['values']['status'];
  $coupon->type = 'price';
  if (substr($form_state['values']['discount'], -1) == '%') {
    $coupon->type = 'percentage';
  }
  if (substr($form_state['values']['discount'], 0, 1) == '=') {
    $coupon->type = 'set_price';
  }
  $coupon->value = str_replace(array('%', '='), '', $form_state['values']['discount']);
  $coupon->minimum_order = $form_state['values']['minimum_order'] ? $form_state['values']['minimum_order'] : 0;
  $coupon->data['minimum_qty'] = $form_state['values']['minimum_qty'];
  $coupon->data['minimum_qty_restrict'] = $form_state['values']['minimum_qty_restrict'];
  $coupon->max_uses = $form_state['values']['max_uses'] ? $form_state['values']['max_uses'] : 0;
  $coupon->data['max_uses_per_user'] = $form_state['values']['max_uses_per_user'];
  $coupon->data['apply_to'] = $form_state['values']['apply_to'];
  if ($form_state['values']['apply_to'] == 'cheapest' || $form_state['values']['apply_to'] == 'expensive') {
    $coupon->data['apply_count'] = $form_state['values']['apply_count'];
  }

  // Map restrictions back to their IDs.
  array_walk($form_state['values']['products'], '_uc_coupon_map_restriction', 'nid');
  array_walk($form_state['values']['terms'], '_uc_coupon_map_restriction', 'tid');
  array_walk($form_state['values']['users'], '_uc_coupon_map_restriction', 'uid');

  // Set coupon restrictions.
  $coupon->data['product_types'] = drupal_map_assoc(array_filter($form_state['values']['product_types']));
  $coupon->data['negate_products'] = $form_state['values']['negate_products'];
  $coupon->data['products'] = drupal_map_assoc(array_filter($form_state['values']['products']));
  $coupon->data['skus'] = drupal_map_assoc(array_filter($form_state['values']['skus']));
  $coupon->data['negate_terms'] = $form_state['values']['negate_terms'];
  $coupon->data['terms'] = drupal_map_assoc(array_filter($form_state['values']['terms']));
  $coupon->data['users'] = drupal_map_assoc(array_filter($form_state['values']['users'], 'is_numeric'));
  $coupon->data['roles'] = drupal_map_assoc(array_filter($form_state['values']['roles']));
  $coupon->data['wholesale'] = $form_state['values']['wholesale'];

  // Remove zero values and empty arrays.
  $coupon->data = array_filter($coupon->data);

  // Handle AJAX submissions.
  if (!empty($form_state['ajax_submission'])) {
    $form_state['coupon_data'] = $coupon->data;
    $form_state['rebuild'] = TRUE;
    return;
  }
  // Clear AJAX storage, or the redirect will fail.
  unset($form_state['storage']['field_count']);

  uc_coupon_save($coupon, $form_state['values']);

  drupal_set_message(t('Coupon %name has been saved.', array('%name' => $coupon->name)));
  $form_state['redirect'] = 'admin/store/coupons' . ($coupon->status ? '' : '/inactive');
}

function _uc_coupon_map_restriction(&$value, $key, $prefix) {
  if ($value && preg_match('/\[' . $prefix . ':(\d+)\]$/', $value, $matches)) {
    $value = $matches[1];
  }
  else {
    $value = FALSE;
  }
}

/**
 * Delete coupon confirm form
 *
 * @param $cid int
 * Coupon ID.
 *
 * @return $confirm
 *  Return a drupal confirm form.
 */
function uc_coupon_delete_confirm($form, &$form_state, $coupon) {
  $form['#uc_coupon_cid'] = $coupon->cid;
  return confirm_form($form, t('Are you sure you want to delete coupon %name with code %code?', array('%name' => $coupon->name, '%code' => $coupon->code)), 'admin/store/coupons', t('This action cannot be undone. Deleting this coupon will remove all records of past uses as well.'), t('Delete'));
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function uc_coupon_delete_confirm_submit($form, &$form_state) {
  $coupon = uc_coupon_load($form['#uc_coupon_cid']);

  module_invoke_all('uc_coupon_delete', $coupon);

  db_delete('uc_coupons')
    ->condition('cid', $form['#uc_coupon_cid'])
    ->execute();
  db_delete('uc_coupons_orders')
    ->condition('cid', $form['#uc_coupon_cid'])
    ->execute();

  drupal_set_message(t('Coupon %name has been deleted.', array('%name' => $coupon->name)));
  $form_state['redirect'] = 'admin/store/coupons' . ($coupon->status ? '' : '/inactive');
}

/**
 * Generate a list of bulk coupon codes.
 */
function uc_coupon_codes_csv($coupon) {
  if (!$coupon->bulk) {
    drupal_not_found();
    return;
  }

  header('Content-Type: application/octet-stream');
  header('Content-Disposition: attachment; filename="' . $coupon->code . '.csv";');

  for ($id = 0; $id < $coupon->data['bulk_number']; $id++) {
    echo uc_coupon_get_bulk_code($coupon, $id) . "\n";
  }
  exit;
}

/**
 * Show the actions a user may perform on a coupon.
 */
function theme_uc_coupon_actions($variables) {
  $coupon = $variables['coupon'];
  $actions = array();
  foreach (module_invoke_all('uc_coupon_actions', $coupon) as $action) {
    $icon = theme('image', array('path' => $action['icon'], 'width' => $action['title']));
    $actions[] = l($icon, $action['url'], array('attributes' => array('title' => $action['title']), 'html' => TRUE));
  }
  return implode(' ', $actions);
}

/**
 * Show coupon code in Views.
 *
 * @param $coupon
 *   The coupon object for which this code is to be formatted.
 *   $coupon->usage can optionally be set.
 *   For bulk coupons, $coupon->code should be set to the specific code to display,.
 */
function theme_uc_coupon_code($variables) {
  $coupon = $variables['coupon'];
  $code = $coupon->code;

  $class = "uc-coupon-code";
  if (isset($coupon->usage)) {
    $uses = $coupon->usage['codes'][$coupon->code];
    if ($coupon->max_uses == 0 || $coupon->max_uses > $uses) {
      $class .= ' uc-coupon-code-available';
      if (arg(0) == 'user') {
        $code = l($code, "user/" . arg(1) . '/coupons/' . $coupon->cid . "/view/$code");
      }
    }
    else {
      $class .= ' uc-coupon-code-used';
    }
  }

  return '<span class="' . $class . '">' . $code . '</span>';
}
