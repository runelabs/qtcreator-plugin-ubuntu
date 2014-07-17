# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
# Copyright 2014 Canonical
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.

""" qtcreator-plugin-ubuntu autopilot tests."""

from qtcreator_plugin_ubuntu.tests import QtCreatorTestCase

from autopilot.matchers import Eventually
from testtools.matchers import Equals
from autopilot.input import Keyboard
from time import sleep

#class CMakeApplicationTest(QtCreatorTestCase):
class QtCreatorPluginTestPlan(QtCreatorTestCase):

    def setUp(self):
        super(QtCreatorPluginTestPlan, self).setUp()

    def test_x86_emulator_creation(self):
       """ Change to the Devices mode and click on the add new emulator button """
       kbd = Keyboard.create("X11")
       kbd.press_and_release('Ctrl+9')
       devices_quickview = self.ide.wait_select_single('QQuickView', source='file:///usr/share/qtcreator/ubuntu/devicespage/main.qml')
       add_emulator_button = devices_quickview.select_single('Button', text='Add Emulator')
       add_emulator_button.visible.wait_for(True)
       """ The simple click_object() moves the pointer to the center of the button, but in some environment the responsive area of the button is smaller """
       self.pointing_device.move(add_emulator_button.globalRect.x + add_emulator_button.width - 1, add_emulator_button.globalRect.y + add_emulator_button.height - 1)
       self.pointing_device.click()
       config_emulator_dialog = devices_quickview.wait_select_single('Dialog', title='Create emulator')
       emulatorname_textfield = config_emulator_dialog.wait_select_single('TextField', placeholderText = 'Emulator name')
       self.pointing_device.click_object(emulatorname_textfield)
       kbd = Keyboard.create("X11")
       kbd.type("TestX86Emulator", delay=0.2)
       create_button = config_emulator_dialog.wait_select_single('Button', text = 'create')
       self.pointing_device.click_object(create_button)
       """ The next step is to enter the password to the pkexec's dialog """
       sleep(2)
       kbd = Keyboard.create("X11")
       kbd.type("Here comes the passwrod", delay=0.2)
       kbd.press_and_release('Enter')
       sleep(10)

    def test_x86_fw1410_click_chroot_creation(self):
       """ Open the Options dialog by triggering the right action """
       action = self.ide.wait_select_single('QAction', text = '&Options...')
       action.slots.trigger()
       setting_dialog = self._get_main_window().wait_select_single('Core::Internal::SettingsDialog')

       """ Select the Ubuntu category and click on the Create Click Target button """
       ubuntu_modelindex = setting_dialog.wait_select_single('QModelIndex', text='Ubuntu')
       self.pointing_device.click_object(ubuntu_modelindex)
       new_target_button = setting_dialog.wait_select_single('QPushButton', text='Create Click Target' )
       self.pointing_device.click_object(new_target_button)

       """ Select the i386 architecture and 14.10 framework in the dialog and push the OK button """
       new_chroot_dialog = self.ide.wait_select_single('Ubuntu::Internal::UbuntuCreateNewChrootDialog')
       arch_combobox = new_chroot_dialog.wait_select_single('QComboBox', objectName = 'comboBoxArch')
       self.pointing_device.click_object(arch_combobox)
       i386_modelindex = new_chroot_dialog.wait_select_single('QModelIndex', text='i386')
       self.pointing_device.click_object(i386_modelindex)
       series_combobox = new_chroot_dialog.wait_select_single('QComboBox', objectName = 'comboBoxSeries')
       self.pointing_device.click_object(series_combobox)
       fw1410_modelindex = new_chroot_dialog.wait_select_single('QModelIndex', text='Framework-14.10')
       self.pointing_device.click_object(fw1410_modelindex)
       button_box = new_chroot_dialog.wait_select_single('QDialogButtonBox', objectName = 'buttonBox')
       ok_pushbutton = button_box.wait_select_single('QPushButton', text='&OK')
       self.pointing_device.click_object(ok_pushbutton)

       """ The next step is to enter the password to the pkexec's dialog """
       sleep(2)
       kbd = Keyboard.create("X11")
       kbd.type("here comes the password", delay=0.2)
       kbd.press_and_release('Enter')

       """ Open the Click run dialog and wait for it finishes the job """
       click_dialog = self.ide.wait_select_single('Ubuntu::Internal::UbuntuClickDialog')
       dialog_button_box = click_dialog.wait_select_single('QDialogButtonBox', objectName = 'buttonBox')
       close_button = dialog_button_box.wait_select_single('QPushButton', text = '&Close')
       while True:
          if(close_button.enabled): break;
          sleep(1)
       output_plaintextedit = click_dialog.wait_select_single('QPlainTextEdit', objectName = 'output')
       self.assertFalse('Click exited with errors, please check the output' in output_plaintextedit.plainText)
       self.assertTrue('Click exited with no errors' in  output_plaintextedit.plainText)
       self.pointing_device.click_object(close_button)

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
