<?php
// $Id$

/**
 * @file
 * Coupon orders count field handler
 */
class uc_coupon_handler_field_all_orders_count_old extends views_handler_field_numeric {

  /**
   * Define option default values.
   */
  function option_definition() {
    $options = parent::option_definition();
    $options['statuses'] = array('default' => 'completed');
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $statuses = array();
    foreach (uc_order_status_list() as $status) {
      $statuses[$status['id']] = $status['title'];
    }

    $form['statuses'] = array(
      '#title' => t('Order statuses'),
      '#type' => 'select',
      '#description' => t('Only orders with selected statuses will be counted.') .'<br />'. t('Hold Ctrl + click to select multiple statuses.'),
      '#options' => $statuses,
      '#default_value' => $this->options['statuses'],
      '#multiple' => TRUE,
      '#size' => 5,
    );
  }

  function pre_render_query($select, $values) {
    $cids = array();
    foreach ($values as $value) {
      $cids[] = $value->{$this->aliases['cid']};
    }
    $sql = "SELECT cid, $select AS tot FROM {uc_coupons_orders} AS co
    	LEFT JOIN {uc_orders} AS o ON (co.oid = o.order_id)
    	WHERE o.order_status IN ('". implode("', '", $this->options['statuses']) ."')
    	AND cid IN (". db_placeholders($cids) .")
    	GROUP BY cid";
    //drupal_set_message($sql);

    $r = db_query($sql, $cids);
    $this->tots = array();
    while ($row = db_fetch_object($r)) {
      $this->tots[$row->cid] = $row->tot;
    }
  }

  function construct() {
    parent::construct();
    $this->additional_fields['cid'] = 'cid';
  }

  function query() {
    $this->ensure_my_table();
    $this->add_additional_fields();
  }

  function render($values) {
    $total = $this->tots[$values->{$this->aliases['cid']}];
    $values->{$this->field_alias} = $total;
    return parent::render($values);
  }

  function pre_render($values) {
    $this->pre_render_query('COUNT(oid)', $values);
    $this->field_alias = "uc_coupons_all_orders_count";
  }
}

