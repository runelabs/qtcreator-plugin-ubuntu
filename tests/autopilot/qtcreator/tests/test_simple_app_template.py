# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2014 Canonical
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

"""qtcreator autopilot tests."""

from qtcreator.tests import QtCreatorTestCase

from autopilot.matchers import Eventually
from testtools.matchers import Equals
from autopilot.input import Keyboard
from time import sleep

class CMakeApplicationTest(QtCreatorTestCase):

    def setUp(self):
        super(CMakeApplicationTest, self).setUp()
        sleep(5)

    def create_new_local_cmake_project(self):
        """Create a cmake project with Tabbed UI + QML plugin against local desktop kit"""
        sleep(5)
        kbd = Keyboard.create("X11")
        kbd.press_and_release('Ctrl+N')
        sleep(1)
        kbd.press_and_release('Tab')
        kbd.press_and_release('Down')
        kbd.press_and_release('Down')
        kbd.press_and_release('Down')
        kbd.press_and_release('Down')
        kbd.press_and_release('Enter')
        kbd.press_and_release('Enter')
        kbd.press_and_release('Enter')
        kbd.press_and_release('Enter')
        sleep(10)
        kbd.press_and_release('Ctrl+r')
        sleep(10)

    def create_new_simple_ui_project_and_deploy_on_device(self):
       """Create a Simple UI QML app"""
       sleep(5)
       kbd = Keyboard.create("X11")
       kbd.press_and_release('Ctrl+N')
       sleep(1)
       kbd.press_and_release('Tab')
       kbd.press_and_release('Enter')
       kbd.press_and_release('Enter')
       kbd.press_and_release('Enter')
       sleep(3)
       kbd.press_and_release('Ctrl+F12')
       sleep(20)

    def create_new_simple_ui_project_and_run_locally(self):
       """Create a Simple UI QML app"""
       sleep(5)
       kbd = Keyboard.create("X11")
       kbd.press_and_release('Ctrl+N')
       self._get_main_window().wait_select_single('Core::Internal::NewDialog')
       sleep(1)
       kbd.press_and_release('Tab')
       button = self._get_new_project_dialog().select_single('QPushButton', text='&Choose...' )
       self.pointing_device.click_object(button)
       next_button = self._get_new_project_wizard_dialog().select_single('QPushButton', text='&Next >')
       self.pointing_device.click_object(next_button)
       finish_button = self._get_new_project_wizard_dialog().select_single('QPushButton', text='&Finish')
       self.pointing_device.click_object(finish_button)
       sleep(3)
       kbd.press_and_release('Ctrl+r')
       sleep(20)


    def create_new_chroot_cmake_project(self):
       """Create a cmake project with Tabbed UI + QML plugin against click chroot kit"""
       sleep(5)
       kbd = Keyboard.create("X11")
       kbd.press_and_release('Ctrl+N')
       sleep(1)
       kbd.press_and_release('Tab')
       kbd.press_and_release('Down')
       kbd.press_and_release('Down')
       kbd.press_and_release('Down')
       kbd.press_and_release('Down')
       kbd.press_and_release('Enter')
       kbd.press_and_release('Enter')
       kbd.press_and_release('Tab')
       kbd.press_and_release('Tab')
       kbd.press_and_release('Space')
       kbd.press_and_release('Tab')
       kbd.press_and_release('Tab')
       kbd.press_and_release('Tab')
       kbd.press_and_release('Space')
       kbd.press_and_release('Tab')
       kbd.press_and_release('Enter')
       kbd.press_and_release('Enter')
       sleep(10)
#       kbd.press_and_release('Ctrl+r')
#       sleep(20)

    def options(self):
       sleep(1)
       kbd = Keyboard.create("X11")
       kbd.press_and_release('Alt+t')
       sleep(1)
       kbd.press_and_release('o')
       setting_dialog = self._get_main_window().wait_select_single('Core::Internal::SettingsDialog')
       kbd.press_and_release('u')
       new_target_button = setting_dialog.select_single('QPushButton', text='Create Click Target' )
       self.pointing_device.click_object(new_target_button)
       sleep(2)

    def test_create_app_with_simple_ui(self):
       """" Open the New File and Project dialog by triggering the right action """
       action = self.ide.wait_select_single('QAction', text = '&New File or Project...')
       action.slots.trigger()
       new_project_dialog = self._get_main_window().wait_select_single('Core::Internal::NewDialog')

       """  Choose the App with Simple UI template in the Ubuntu category  """
       ubuntu_modelindex = new_project_dialog.wait_select_single('QModelIndex', text='  Ubuntu')
       self.pointing_device.click_object(ubuntu_modelindex)
       app_with_simple_ui_modelindex = new_project_dialog.wait_select_single('QModelIndex', text='App with Simple UI')
       self.pointing_device.click_object(app_with_simple_ui_modelindex)
       choose_pushbutton = new_project_dialog.wait_select_single('QPushButton', text='Choose...')
       self.pointing_device.click_object(choose_pushbutton)
       application_wizard_dialog = self._get_main_window().wait_select_single('Ubuntu::Internal::UbuntuProjectApplicationWizardDialog')

       """ Clear the default project name and enter the test name to the edit line and hit the Next->Next->Finish buttons """
       projectname_lineedit = application_wizard_dialog.wait_select_single('Utils::ProjectNameValidatingLineEdit')
       kbd = Keyboard.create("X11")
       kbd.press_and_release('Ctrl+A')
       kbd.press_and_release('Delete')
       with kbd.focused_type(projectname_lineedit) as kb:
              kb.type("appwithsimpleui")
              self.assertThat(projectname_lineedit.text, Equals("appwithsimpleui"))
       next_pushbutton = application_wizard_dialog.wait_select_single('QPushButton', text = '&Next >')
       self.pointing_device.click_object(next_pushbutton)
       next_pushbutton = application_wizard_dialog.wait_select_single('QPushButton', text = '&Next >')
       self.pointing_device.click_object(next_pushbutton)
       next_pushbutton = application_wizard_dialog.wait_select_single('QPushButton', text = '&Finish')
       self.pointing_device.click_object(next_pushbutton)

       """ Change to the Publish mode and click on the Create package button"""
       kbd.press_and_release('Ctrl+6')
       fancy_tab_widget = self._get_main_window().wait_select_single('Core::Internal::FancyTabWidget')
       packaging_widget = fancy_tab_widget.wait_select_single('UbuntuPackagingWidget', objectName = 'UbuntuPackagingWidget')
       packaging_groupbox = packaging_widget.wait_select_single('QGroupBox', objectName = 'groupBoxPackaging')
       click_package_pushbutton = packaging_groupbox.wait_select_single('QPushButton', objectName = 'pushButtonClickPackage')
       self.pointing_device.click_object(click_package_pushbutton)

       """ I do not know how to figure out when the click package check is done """
       sleep(10)

       """ Check the error type if there was any error during the package creation """
       validation_groupbox = packaging_widget.wait_select_single('QGroupBox', objectName = 'groupBoxValidate')
       errorinfo_groupbox = validation_groupbox.wait_select_single('QGroupBox', objectName = 'groupBoxErrorInfo')
       errortype_label = errorinfo_groupbox.wait_select_single('QLabel', objectName = 'labelErrorType')
       self.assertThat(errortype_label.text, Equals(""))

    def test_plugins(self):
        """ Open the About Plugins dialog """
        action = self.ide.wait_select_single('QAction', text='About &Plugins...')
        action.slots.trigger()

        """ Check for each Ubuntu specific plugin in the plugin tree """
        plugin_dialog = self._get_main_window().wait_select_single('Core::Internal::PluginDialog')
        ubuntu_treewidgetitem = plugin_dialog.wait_select_single('QTreeWidgetItem', text='Ubuntu')
        cmake_treewidgetitem = plugin_dialog.wait_select_single('QTreeWidgetItem', text='CMakeProjectManager')
        remotelinux_treewidgetitem = plugin_dialog.wait_select_single('QTreeWidgetItem', text='RemoteLinux')
        golang_treewidgetitem = plugin_dialog.wait_select_single('QTreeWidgetItem', text='GoLang')
