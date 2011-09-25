<?php

function uc_coupon_entity_info() {
  return array(
    'uc_coupon' => array(
      'label' => t('Coupon'),
      'controller class' => 'UcCouponController',
      'base table' => 'uc_coupons',
      'fieldable' => TRUE,
      'entity keys' => array(
        'id' => 'cid',
      ),
      'bundles' => array(
        'uc_coupon' => array(
          'label' => t('Coupon'),
          'admin' => array(
            'path' => 'admin/store/settings/coupon',
            'access arguments' => array('manage store coupons'),
          ),
        ),
      ),
      'view modes' => array(
        'full' => array(
          'label' => t('Administrative view'),
        ),
      ),
    ),
  );
}

/**
 * Loads one coupon entity from the database.
 */
function uc_coupon_load($cid, $reset = FALSE) {
  if (is_null($cid) || $cid < 1) {
    return FALSE;
  }

  $coupons = uc_coupon_load_multiple(array($cid), array(), $reset);

  return $coupons ? reset($coupons) : FALSE;
}

/**
 * Loads one or more coupon entities from the database.
 *
 * @param $ids
 *   An array of coupon IDs.
 * @param $conditions
 *   An array of conditions on the {uc_coupons} table in the form
 *  'field' => $value.
 *
 * @return
 *   An array of order objects indexed by order_id.
 */
function uc_coupon_load_multiple($ids, $conditions = array(), $reset = FALSE) {
  return entity_load('uc_coupon', $ids, $conditions, $reset);
}


/**
 * Controller class for uc_order entity.
 */
class UcCouponController extends DrupalDefaultEntityController {

  function attachLoad(&$coupons, $revision_id = FALSE) {
    foreach ($coupons as &$coupon) {
      $coupon->data = isset($coupon->data) ? unserialize($coupon->data) : array();
      // Allow other modules to alter the coupon data.
      drupal_alter('uc_coupon', $coupon);
    }
    parent::attachLoad($coupons, $revision_id);
  }

}

/**
 * Save a coupon object.
 *
 * If the 'cid' field is set, then this will update an existing coupon.
 * Otherwise, a new bulk seed will be generated, the coupon will be
 * inserted into the database, and $coupon->cid will be set.
 *
 * @param $coupon
 *   The coupon to save.
 *
 * @param $edit
 *   An optional array of extra data that other modules may need to save.
 */
function uc_coupon_save(&$coupon, $edit = array()) {
  $transaction = db_transaction();
  try {
    field_attach_presave('uc_coupon', $coupon);

    // Allow other modules to alter the coupon before saving.
    foreach (module_implements('uc_coupon_presave') as $module) {
      $callback = $module . '_uc_coupon_presave';
      $callback($coupon, $edit);
    }
    if (!empty($coupon->cid)) {
      drupal_write_record('uc_coupons', $coupon, 'cid');
      field_attach_update('uc_coupon', $coupon);
    }
    else {
      $coupon->created = REQUEST_TIME;
      $coupon->bulk_seed = md5(uniqid());
      drupal_write_record('uc_coupons', $coupon);
      field_attach_insert('uc_coupon', $coupon);
    }

    // Notify other modules that a coupon has been saved.
    module_invoke_all('uc_coupon_save', $coupon);
  }
  catch (Exception $e) {
    $transaction->rollback('uc_order');
    watchdog_exception('uc_order', $e);
    throw $e;
  }
}

function uc_coupon_delete($cid) {
  $coupon = uc_coupon_load($cid);

  module_invoke_all('uc_coupon_delete', $coupon);

  db_delete('uc_coupons')
    ->condition('cid', $cid)
    ->execute();
  db_delete('uc_coupons_orders')
    ->condition('cid', $cid)
    ->execute();
    
  return $coupon;
}


/**
 * Implements hook_field_extra_fields().
 */
function uc_coupon_field_extra_fields() {
  $extra = array();

  $extra['uc_coupon']['uc_coupon']['display']['admin_summary'] = array(
    'label' => t('Administrative Summary'),
    'description' => t('A summary of all coupon details.'),
    'weight' => 0,
  );

  return $extra;
}
