<?php
// $Id$

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
  else if (time() < $coupon->valid_from) {
    $status = t('Not valid until @date', array('@date' => format_date($coupon->valid_from, 'custom', variable_get('uc_date_format_default', 'm/d/Y'))));
  }
  else if (time() > $coupon->valid_until) {
    $status = t('Expired on @date', array('@date' => format_date($coupon->valid_until, 'custom', variable_get('uc_date_format_default', 'm/d/Y'))));
  }
  else {
    $status = t('Active until @date', array('@date' => format_date($coupon->valid_until, 'custom', variable_get('uc_date_format_default', 'm/d/Y'))));
  }
  $rows[] = array(t('Status'), $status);

  if (!$coupon->bulk) {
    $rows[] = array(t('Code'), check_plain($coupon->code));
    if ($coupon->max_uses) {
      $rows[] = array(t('Maximum uses'), $coupon->max_uses);
    }
  }
  else {
    $rows[] = array(t('Codes'), check_plain($coupon->code) .' &times; '. check_plain($coupon->data['bulk_number']) .' ('. l(t('Download as CSV'), 'admin/store/customers/coupon/'. $coupon->cid .'/codes') .')');
    //$rows[] = array(t('Bulk seed'), check_plain($coupon->bulk_seed));
    if ($coupon->max_uses) {
      $rows[] = array(t('Maximum uses per code'), $coupon->max_uses);
    }
  }

  $value = ($coupon->type == 'percentage') ? ((float)$coupon->value .'%') : uc_currency_format($coupon->value);
  $rows[] = array(t('Discount value'), $value);

  if (isset($coupon->data['products']) || isset($coupon->data['skus']) || isset($coupon->data['terms']) || isset($coupon->data['product_types'])) {
    if (isset($coupon->data['max_applicable_products']) && ($num = $coupon->data['max_applicable_products']) > 0) {
      if ($coupon->data['max_applicable_products_value'] == 'cheapest') {
        $applies = format_plural($num, 'Cheapest product', '@count cheapest products');
      }
      else {
        $applies = format_plural($num, 'Most expensive product', '@count most expensive products');
      }
    }
    else {
      $applies = t('Each product');
    }
    $rows[] = array(t('Applied to'), $applies);

    $restrict = array();
    if (isset($coupon->data['product_types'])) {
      $key = format_plural(count($coupon->data['product_types']), 'All products in class', 'All products in classes');
      $restrict[$key] = $coupon->data['product_types'];
    }
    if (isset($coupon->data['products'])) {
      $products = array();
      foreach ($coupon->data['products'] as $nid) {
        $products[] = check_plain(db_result(db_query("SELECT title FROM {node} WHERE nid = %d", $nid)));
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
        $terms[] = check_plain(db_result(db_query("SELECT name FROM {term_data} WHERE tid = %d", $tid)));
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
          $title = t('or') .' '. $title;
        }
        $restriction = $title .': <em>'. implode('</em>, <em>', $restriction) .'</em>';
        $or = TRUE;
      }
      $rows[] = array(t('Product restrictions'), implode($restrict, '<br />'));
    }
  }
  else {
    $rows[] = array(t('Applied to'), t('Order subtotal'));
  }

  $restrict = array();
  if (isset($coupon->data['users'])) {
    $users = array();
    foreach ($coupon->data['users'] as $uid) {
      $users[] = check_plain(db_result(db_query("SELECT name FROM {users} WHERE uid = %d", $uid)));
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
  else if ($coupon->data['wholesale'] == 3) {
    $restrict['Does not have permission'] = array('coupon wholesale pricing');
  }
  if ($restrict) {
    foreach ($restrict as $title => &$restriction) {
      $restriction = $title .': <em>'. implode('</em>, <em>', $restriction) .'</em>';
    }
    $rows[] = array(t('User restrictions'), implode($restrict, '<br />'));
  }

  if ($coupon->minimum_order > 0) {
    $rows[] = array(t('Order restrictions'), t('Minimum subtotal') .': <em>'. uc_currency_format($coupon->minimum_order) .'</em>');
  }

  foreach ($rows as &$row) {
    $row[0] = array('header' => TRUE, 'data' => $row[0]);
  }

  $output = theme('table', array(), $rows);
  return $output;
}
