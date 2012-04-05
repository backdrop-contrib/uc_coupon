<?php

/**
 * Implements hook_entity_info();
 */
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
 * Controller class for uc_coupon entity.
 */
class UcCouponController extends EntityAPIController {
  /**
   * @see EntityAPIController::delete()
   */
  public function delete($ids, DatabaseTransaction $transaction = NULL) {
    parent::delete($ids, $transaction);
    db_delete('uc_coupons_orders')
    ->condition('cid', $ids, 'IN')
    ->execute();
  }
  
  /**
   * @see EntityAPIController::save()
   */
  public function save($coupon, DatabaseTransaction $transaction = NULL) {
    if (empty($coupon->cid)) {
      $coupon->created = REQUEST_TIME;
      $coupon->bulk_seed = md5(uniqid());
    }
    parent::save($coupon, $transaction);
  }

  /**
   * @see EntityAPIController::buildContent()
   */
  public function buildContent($coupon, $view_mode = 'full', $langcode = NULL, $content = array()) {
    $rows = array();
    $rows[] = array(t('Name'), check_plain($coupon->name));

    if (!$coupon->status) {
      $status = t('Inactive');
    }
    elseif (!$coupon->valid_until) {
      $status = t('Active');
    }
    elseif (REQUEST_TIME < $coupon->valid_from) {
      $status = t('Not valid until @date', array('@date' => _uc_coupon_format_date($coupon->valid_from, variable_get('date_format_uc_store', 'm/d/Y H:iT'))));
    }
    elseif (REQUEST_TIME > $coupon->valid_until) {
      $status = t('Expired on @date', array('@date' => _uc_coupon_format_date($coupon->valid_until, variable_get('date_format_uc_store', 'm/d/Y H:iT'))));
    }
    else {
      $status = t('Active until @date', array('@date' => _uc_coupon_format_date($coupon->valid_until, variable_get('date_format_uc_store', 'm/d/Y H:iT'))));
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

    $rows[] = array(t('Discount value'), theme('uc_coupon_discount', array('coupon' => $coupon)));

    switch ($coupon->data['apply_to']) {
      case 'subtotal':
        $applies = t('Order subtotal');
        break;
      case 'products_total':
        $applies = t('Total of matching products');
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
      if (isset($coupon->data['negate_users'])) {
        $restrict[t('All users except')] = $users;
      }
      else {
        $restrict[format_plural(count($users), 'User', 'Users')] = $users;
      }
    }
    if (isset($coupon->data['max_uses_per_user'])) {
      $restrict['Maximum uses per user'] = array($coupon->data['max_uses_per_user']);
    }
    if (isset($coupon->data['roles'])) {
      if (isset($coupon->data['negate_roles'])) {
        $restrict[t('All roles except')] = $coupon->data['roles'];
      }
      else {
        $restrict[format_plural(count($users), 'Role', 'Roles')] = $coupon->data['roles'];
      }
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

    $combo_list = array();
    if (!empty($coupon->data['combinations'])) {
      $combos = db_query('SELECT cid, name FROM {uc_coupons} where cid IN (:cids)', array(':cids' => $coupon->data['combinations']));
      foreach ($combos as $combo) {
        $combo_list[] = check_plain("$combo->name [cid:$combo->cid]");
      }
    }
    if (isset($coupon->data['negate_combinations'])) {
      $ctext = empty($combo_list) ? t('Any.') : t('Any but:') . ' ' . implode(', ', $combo_list);
    }
    else {
      $ctext = empty($combo_list) ? t('None.') : t('Only:') . ' ' . implode(', ', $combo_list);
    }
    $rows[] = array(t('Allowed Combinations'), $ctext);

    foreach ($rows as &$row) {
      $row[0] = array(
        'header' => TRUE,
        'data' => $row[0],
      );
    }

    $content['admin_summary'] = array(
      '#title' => t('Administrative Summary'),
      '#theme' => 'table',
      '#rows' => $rows
    );

    return parent::buildContent($coupon, $view_mode, $langcode, $content);
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
  entity_save('uc_coupon', $coupon);
}

/**
 * Delete a coupon object.
 *
 * @param $cid
 *   The id of the coupon to delete.
 */
function uc_coupon_delete($cid) {
  entity_delete('uc_coupon', $cid);
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
