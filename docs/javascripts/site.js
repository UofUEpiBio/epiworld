(function () {
  "use strict";

  function initNavToggle() {
    var button = document.querySelector(".site-nav-toggle");
    var sidebar = document.getElementById("site-sidebar");

    if (!button || !sidebar) {
      return;
    }

    button.addEventListener("click", function () {
      var expanded = button.getAttribute("aria-expanded") === "true";
      button.setAttribute("aria-expanded", expanded ? "false" : "true");
      document.body.classList.toggle("nav-open", !expanded);
    });
  }

  document.addEventListener("DOMContentLoaded", initNavToggle);
})();
