<?php
// $Id$

/**
 * @file
 * Views 2 hooks and callback registries.
 */

/**
 * Implementation of hook_views_data().
 */
function uc_coupon_views_data() {
  $data['uc_coupons']['table']['group'] = t('Coupon');

  $data['uc_coupons']['table']['base'] = array(
    'field' => 'cid',
    'title' => t('Coupon'),
    'help' => t("Ubercart discount coupons."),
  );

  $data['uc_coupons']['cid'] = array(
    'title' => t('ID'),
    'help' => t('The unique ID of the coupon.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_numeric',
    ),
  );

  $data['uc_coupons']['name'] = array(
    'title' => t('Name'),
    'help' => t('The name of the coupon.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_string',
    ),
  );

  $data['uc_coupons']['code'] = array(
    'title' => t('Code'),
    'help' => t('The coupon code, or prefix for bulk coupons.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_string',
    ),
  );

  $data['uc_coupons']['bulk'] = array(
    'title' => t('Bulk'),
    'help' => t('Whether the coupon is a bulk coupon with multiple suffixes.'),
    'field' => array(
      'handler' => 'views_handler_field_boolean',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_boolean_operator',
      'label' => t('Is a bulk coupon'),
      'type' => 'yes-no',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['uc_coupons']['created'] = array(
    'title' => t('Creation date'),
    'help' => t('The date the coupon was created.'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_date',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
    ),
  );

  $data['uc_coupons']['valid_from'] = array(
    'title' => t('Valid from'),
    'help' => t('The date the coupon becomes valid.'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_date',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
    ),
  );

  $data['uc_coupons']['valid_until'] = array(
    'title' => t('Valid until'),
    'help' => t('The date the coupon expires.'),
    'field' => array(
      'handler' => 'views_handler_field_date',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort_date',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_date',
    ),
  );

  $data['uc_coupons']['status'] = array(
    'title' => 'Active',
    'help' => t('Whether the coupon is available to use.'),
    'field' => array(
      'handler' => 'views_handler_field_boolean',
      'click sortable' => TRUE,
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_boolean_operator',
      'label' => t('Active'),
      'type' => 'yes-no',
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );

  $data['uc_coupons']['type'] = array(
    'title' => 'Type',
    'help' => t('The discount type of the coupon.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_string',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_string',
    ),
  );

  $data['uc_coupons']['value'] = array(
    'title' => t('Value'),
    'help' => t('The value of the coupon.'),
    'field' => array(
      'handler' => 'uc_coupon_handler_field_value',
      'click sortable' => TRUE,
      'float' => TRUE,
      'additional fields' => array(
        'type' => 'type',
      ),
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_float',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_float',
    ),
  );

  $data['uc_coupons']['minimum_order'] = array(
    'title' => t('Minimum order total'),
    'help' => t('The minimum order total that applies to the coupon.'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'click sortable' => TRUE,
      'float' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_float',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_float',
    ),
  );

  $data['uc_coupons']['max_uses'] = array(
    'title' => t('Max uses per code'),
    'help' => t('The maximum number of times this coupon can be redeemed.'),
    'field' => array(
      'handler' => 'views_handler_field',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
    'filter' => array(
      'handler' => 'views_handler_filter_numeric',
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_numeric',
    ),
  );

  $data['uc_coupons']['actions'] = array(
    'title' => t('Actions'),
    'help' => t('A set of actions the current user can perform on the coupon.'),
    'field' => array(
      'handler' => 'uc_coupon_handler_field_actions',
      'field' => 'cid',
      'additional fields' => array(
        'name' => 'name',
        'bulk' => 'bulk',
      ),
    ),
  );

  return $data;
}

/**
 * Implementation of hook_views_handlers().
 */
function uc_coupon_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'uc_coupon') .'/views',
    ),
    'handlers' => array(
      'uc_coupon_handler_field_value' => array(
        'parent' => 'views_handler_field_numeric',
      ),
      'uc_coupon_handler_field_actions' => array(
        'parent' => 'views_handler_field',
      ),
    ),
  );
}
