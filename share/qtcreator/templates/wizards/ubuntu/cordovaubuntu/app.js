/* Create UI instance */

var UI = new UbuntuUI();

/* Initializatino after window load */
window.onload = function () {
    /* REQUIRED: initialize UI */
    UI.init();
    
    /* Start pagestack with main page */
    UI.pagestack.push("main");

    /* GUI intial state: show loading dialog (is removed on deviceready event) */
    UI.dialog("loading").show();

    /* Connect button to function */
    UI.button("devicereadyOK").click(function() {
        UI.dialog("deviceready").hide();
    });

};

/* Respond to deviceready event */
document.addEventListener("deviceready", function() {
    UI.dialog("loading").hide();
    UI.dialog("deviceready").show();
    }, false);
