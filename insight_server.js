/**
 * @file
 * Attaches the JS behaviors for the Insight Server UI.
 */

(function ($) {
    /**
     * Redirects a user to the selected project page.
     */
    Drupal.behaviors.insightServerProjectRedirect = {
        attach: function (context, settings) {
            $('#insight-server-dashboard-form #edit-projects').change(function() {
                if (this.value != '') {
                    window.location.href = this.value;
                }
            })
        }
    };
})(jQuery);
