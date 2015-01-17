import QtQuick 2.0
import QtTest 1.0
import Ubuntu.Test 1.0
import "../../"
// See more details @ https://developer.ubuntu.com/api/qml/sdk-14.10/Ubuntu.Test.UbuntuTestCase

// Execute tests with:
//   qmltestrunner

Item {

    width: units.gu(100)
    height: units.gu(75)

    // The objects
    Main {
        id: main
    }

    UbuntuTestCase {
        name: "MainTestCase"

        when: windowShown

        function initTestCase() {
            console.debug(">> initTestCase");
            console.debug("<< initTestCase");
        }

        function cleanupTestCase() {
            console.debug(">> cleanupTestCase");
            console.debug("<< cleanupTestCase");
        }

        function init() {
            console.debug(">> init");
            var label = findChild(main, "label");
            compare("Hello..", label.text, "Text was not the expected.");
            console.debug("<< init");
        }

        function cleanup() {
            console.debug(">> cleanup");
            console.debug("<< cleanup");
        }

        function test_clickButtonMustChangeLabel() {
            var button = findChild(main, "button");
            var buttonCenter = centerOf(button)
            mouseClick(button, buttonCenter.x, buttonCenter.y);
            var label = findChild(main, "label");
            tryCompare(label, "text", "..world!", 1, "Text was not the expected.");
        }
    }
}
