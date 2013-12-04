window.onload = function () {
    var UI = new UbuntuUI();
    UI.init();

    UI.button('tapme').click(function () {
	UI.element('#mytext').innerText = '..world!';
    });
};

