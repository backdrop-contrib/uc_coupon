// $Id$

Drupal.behaviors.ucCouponAdmin = function(context) {
  $('#edit-discount', context).keyup(function() {
    if (this.value.indexOf('%') == -1) {
      $(this).siblings('span').show();
    }
    else {
      $(this).siblings('span').hide();
    }
  });

  $('input[name=apply_to]', context).click(function() {
    if (this.value == 'cheapest' || this.value == 'expensive') {
      $('#edit-apply-count-wrapper').show();
    }
    else {
      $('#edit-apply-count-wrapper').hide();
    }
  }).filter(':checked').click();
}
