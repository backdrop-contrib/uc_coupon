// $Id$

Drupal.behaviors.ucCouponAdmin = function(context) {
  $('input[name=apply_to]', context).click(function() {
    if (this.value == 'cheapest' || this.value == 'expensive') {
      $('#edit-apply-count-wrapper').show();
    }
    else {
      $('#edit-apply-count-wrapper').hide();
    }
  }).filter(':checked').click();
}
