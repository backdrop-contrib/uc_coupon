# Ubercart Coupons

Provides discount codes and gift certificates for Ubercart stores.

Coupons are configured at Store administration > Coupons. Global settings for
the module are configured at Store administration > Configuration > Coupon
module settings.

A checkout pane, a cart pane and a block are available for customers to enter
coupon codes, each of which can be enabled or disabled separately. Discounts can
be shown as an item in the cart or applied on the checkout page.

Coupons can apply either a fixed price discount or a percentage discount to the
order subtotal or to selected product prices. Coupons can be restricted to apply
to specific products, product classes, SKUs, or taxonomy terms. Coupons can also
be configured so they only apply between certain dates, a fixed number of times,
when a fixed order quantity or order subtotal is reached, to specified users or
roles, or any combination of these.

Bulk coupon codes can be created, each of which is given a randomised code,
which can be individually restricted and distributed to customers as necessary.

A new report is added to the Ubercart reports page, showing you which coupons
were used on specific orders and the total discounts that were applied.

In European stores where VAT is applied, the latest version of [uc_vat](https://www.drupal.org/project/uc_vat) now
ensures discounts are now correctly calculated when product prices include tax.

## Discount Coupon Purchase module

This submodule allows new coupons to be generated when a specific product is
purchased. "Coupon creation" can be added to any product as a product feature.
After checkout, a new coupon will be created and the code(s) will be emailed to
the purchaser.

## Notes

If you are providing 100% discounts using coupons, you may also need
[Ubercart Custom Payments](http://drupal.org/project/uc_custom_payment) to skip payment
screens at checkout.

## Support

Bugs and Feature requests should be reported in the [Issue queue](https://github.com/backdrop-contrib/uc_coupon/issues).

## Maintainers

- [Herb v/d Dool](https://github.com/herbdool/)
- This module is seeking additional maintainers.

## Credits

Ported to Backdrop CMS by Herb v/d Dool.

Originally written for Drupal by

- [longwave](https://www.drupal.org/u/longwave)
- [BlakeLucchesi](https://www.drupal.org/u/blakelucchesi)
- [amorsent](https://www.drupal.org/u/amorsent)
- [wodenx](https://www.drupal.org/u/wodenx)

## License

This project is GPL v3 software. See the LICENSE.txt file in this directory for
complete text.
