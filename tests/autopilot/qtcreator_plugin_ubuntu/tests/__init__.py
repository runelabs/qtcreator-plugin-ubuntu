# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2014 Canonical
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""qtcreator autopilot tests."""

from autopilot.input import Mouse, Pointer
from autopilot.testcase import AutopilotTestCase

import shutil
import tempfile
import os

class QtCreatorTestCase(AutopilotTestCase):

    def setUp(self):
        self.pointing_device = Pointer(Mouse.create())
        self._set_temporary_home_directory()
        super(QtCreatorTestCase, self).setUp()
        self.launch_qt_creator()

    def launch_qt_creator(self):
#        self.patch_environment('HOME','/home/balogh')
        self.ide = self.launch_test_application('qtcreator')

    def _create_temporary_directory(self):
        self.temporary_directory = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temporary_directory)

    def _set_temporary_home_directory(self):
        self._create_temporary_directory()

 #       self.patch_environment('HOME', self.temporary_directory)

    def _get_main_window(self):
        return self.ide.wait_select_single(
            'Core::Internal::MainWindow'
        )

    def _get_welcome_tab_view(self):
        return self.ide.wait_select_single(
            'QWidgetWindow', objectName='Core::Internal::MainWindowClassWindow'
        )

    def _get_left_tabbar(self):
        main_window = self._get_main_window()
        return main_window.select_single(
            'Core::Internal::FancyTabBar'
        )

    def _get_number_of_tabs(self):
        tabs = self._get_left_tabbar().select_many(
            'Core::Internal::FancyTab'
        )
        return len(tabs)

    def _get_current_active_tab_name(self):
        return self._get_left_tabbar().selectedTabLabel

    def _get_new_project_button(self):
        return self._get_welcome_tab_view().wait_select_single(
            'Button', text='New Project'
        )

    def _get_new_project_dialog(self):
        return self._get_main_window().wait_select_single(
            'Core::Internal::NewDialog'
        )

    def _click_new_project_dialog_choose_button(self):
        button = self._get_new_project_dialog().select_single(
            'QPushButton', text='&Choose...'
        )

        self.pointing_device.click_object(button)

    def _get_new_project_wizard_dialog(self):
        return self.ide.wait_select_single(
            'Ubuntu::Internal::UbuntuProjectApplicationWizardDialog'
        )

    def _get_new_project_name_input_field(self):
        return self._get_new_project_wizard_dialog().select_single(
            'Utils::ProjectNameValidatingLineEdit'
        )

    def _get_new_project_location_field(self):
        return self._get_new_project_wizard_dialog().select_single(
            'Utils::PathChooser'
        )

    def _clear_input_field(self, input_field):
        self.pointing_device.click_object(input_field)

        self.pointing_device.click()
        self.pointing_device.click()
        self.keyboard.press_and_release('Backspace')

    def _type_new_project_name(self, project):
        input_field = self._get_new_project_name_input_field()
        self._clear_input_field(input_field)
        
        self.keyboard.type(project)

    def _type_new_project_location(self, location):
        location_field = self._get_new_project_location_field()
        self._clear_input_field(location_field)

        self.keyboard.type(location)

    def click_new_project_button(self):
        new_button = self._get_new_project_button()

        self.pointing_device.click_object(new_button)

        return self._get_new_project_dialog()

    def _get_wizard_finish_button(self):
        return self._get_new_project_wizard_dialog().select_single(
            'QPushButton', text='&Finish')

    def _get_wizard_next_button(self):
        return self._get_new_project_wizard_dialog().select_single(
            'QPushButton', text='&Next >')

    def _click_wizard_finish_button(self):
        finish_button = self._get_wizard_finish_button()

        self.pointing_device.click_object(finish_button)

    def _click_wizard_next_button(self):
        next_button = self._get_wizard_next_button()

        self.pointing_device.click_object(next_button)

    def switch_to_tab_by_name(self, tab_name):
        current_tab = self._get_current_active_tab_name()
        if tab_name == current_tab:
            return

        tabbar = self._get_left_tabbar()
        tabbar_height = tabbar.height
        number_of_tabs = self._get_number_of_tabs()
        tbar_x, tbar_y, tbar_width, tbar_height = tabbar.globalRect

        tab_number = 1
        while current_tab != tab_name and not tab_number > number_of_tabs:
            tab_center = ((tabbar_height / number_of_tabs) * tab_number) - \
                         ((tabbar_height / number_of_tabs) / 2)
            tx = tbar_x + tbar_width / 2
            ty = tbar_y + tab_center
            self.pointing_device.move(tx, ty)
            self.pointing_device.click()

            current_tab = self._get_current_active_tab_name()
            tab_number += 1
