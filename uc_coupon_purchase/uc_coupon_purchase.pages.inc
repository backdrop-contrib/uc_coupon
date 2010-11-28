<?php
// $Id$

/**
 * Display a list of purchased coupons.
 */
function uc_coupon_purchase_view($account) {
  drupal_set_title(t('My coupons'));

  $result = db_query('SELECT c.* FROM {uc_coupon_purchase_users} u INNER JOIN {uc_coupons} c ON u.cid = c.cid WHERE u.uid = %d ORDER BY valid_until ASC, code ASC', $account->uid);
  
  $header = array(t('Code'), t('Value'), t('Validity'), t('Max uses'), t('Used'));
  $rows = array();
  while ($coupon = db_fetch_object($result)) {
    if ($coupon->bulk) {
      $coupon->data = unserialize($coupon->data);
      $codes = array();
      for ($id = 0; $id < $coupon->data['bulk_number']; $id++) {
        $codes[] = uc_coupon_get_bulk_code($coupon, $id);
      }
    }
    else {
      $codes = array($coupon->code);
    }

    $value = $coupon->type == 'percentage' ? $coupon->value . '%' : uc_currency_format($coupon->value);
    $valid_from = format_date($coupon->valid_from, 'custom', variable_get('uc_date_format_default', 'm/d/Y'));
    $valid_until = format_date($coupon->valid_until, 'custom', variable_get('uc_date_format_default', 'm/d/Y'));
    $valid = $coupon->status ? "$valid_from - $valid_until" : t('Inactive');
    $uses = $coupon->max_uses > 0 ? $coupon->max_uses : t('Unlimited');
    
    foreach ($codes as $code) {
      $used = db_result(db_query("SELECT COUNT(*) FROM {uc_coupons_orders} AS uco LEFT JOIN {uc_orders} AS uo ON uco.oid = uo.order_id LEFT JOIN {uc_order_statuses} AS uos ON uo.order_status = uos.order_status_id WHERE uos.weight > 0 AND uco.cid = %d AND uco.code = '%s'", $coupon->cid, $code));
      $rows[] = array($code, $value, $valid, $uses, $used);
    }
  }

  if (count($rows)) {
    $output = "<p>" . t("The table below lists the coupons you have purchased.  Please take care that you do not provide any one code to more customers than are allowed in the 'max uses' column") . "</p>";
    $output .= theme('table', $header, $rows);
  }
  else {
    $output = "<p>" . t('You currently have no coupons available.') . "</p>";
  }
  
  return $output;
}
