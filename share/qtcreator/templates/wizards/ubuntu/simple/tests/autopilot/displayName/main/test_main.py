# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-

"""Tests for the Hello World"""

import os

from autopilot.matchers import Eventually
from testtools.matchers import Equals

import %DISPLAYNAME%


class MainViewTestCase(%DISPLAYNAME%.ClickAppTestCase):
    """Generic tests for the Hello World"""

    test_qml_file_path = "%s/%s.qml" % (os.path.dirname(os.path.realpath(__file__)),"../../../../%DISPLAYNAME%")

    def test_inititial_label(self):
        label = self.main_view.select_single(objectName="label")
        self.assertThat(label.text, Equals("Hello.."))

    def test_click_button_should_update_label(self):
        button = self.main_view.select_single(objectName="button")
        self.pointing_device.click_object(button)
        label = self.main_view.select_single(objectName="label")
        self.assertThat(label.text, Eventually(Equals("..world!")))
