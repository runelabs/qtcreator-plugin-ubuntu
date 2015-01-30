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
import re

#class CMakeApplicationTest(QtCreatorTestCase):
class QtCreatorPluginTestPlan(QtCreatorTestCase):

    def setUp(self):
        super(QtCreatorPluginTestPlan, self).setUp()

# tools -> options -> Build & Run -> Kits -> ".* for armhf (GCC ubuntu-sdk-14.10-utopic)"
    def test_existing_kits(self):
        """ Open the Options dialog by triggering the right action """
        action = self.ide.wait_select_single('QAction', text = '&Options...')
        action.slots.trigger()
        setting_dialog = self._get_main_window().wait_select_single('Core::Internal::SettingsDialog')

        """ Se/lect the Ubuntu category and click on the Create Click Target button """
        ubuntu_modelindex = setting_dialog.wait_select_single('QModelIndex', text='Build & Run')
        self.pointing_device.click_object(ubuntu_modelindex)


        for index, whatever in enumerate(setting_dialog.select_many('QItemSelectionModel')):
            print whatever.text

        for index, kit in enumerate(setting_dialog.select_many('QModelIndex')):
            if re.search('GCC ubuntu-sdk', kit.text):
                print kit.text

#       kit_modelindex = setting_dialog.wait_select_single('QModelIndex', text='UbuntuSDK for armhf (GCC ubuntu-sdk-14.10-utopic)')
#       kit_modelindex = setting_dialog.wait_select_single('QModelIndex', text=~'.*armhf.*')
#       self.pointing_device.click_object(kit_modelindex)

# QWidget ->  QTreeView -> 
        sleep(2)

    def test_create_app_with_backend(self):
        self._createAndOpenProject('App with QML Extension Library',"appwithbackend")
        sleep(20)

        """ Try to build the project """
        trojanHorse = self.ide.wait_select_single('Ubuntu::Internal::UbuntuTestControl')
        sigwatch = trojanHorse.watch_signal("buildFinished()")
        trojanHorse.slots.triggerCommand("ProjectExplorer.Build")
        self.assertThat(lambda: sigwatch.was_emitted, Eventually(Equals(True)))
        sleep(1)
        self.assertTrue(trojanHorse.lastBuildSuccess)


    def test_create_app_with_simple_ui(self):
        self._createAndOpenProject('App with Simple UI',"appwithsimpleui")
        """ Change to the Publish mode and click on the Create package button"""
        fancy_tab_widget = self._get_main_window().wait_select_single('Core::Internal::FancyTabWidget')
        fancy_tab_widget.slots.setCurrentIndex(5)
        packaging_widget = fancy_tab_widget.wait_select_single('UbuntuPackagingWidget', objectName = 'UbuntuPackagingWidget')
        click_package_pushbutton = packaging_widget.wait_select_single('QPushButton', objectName = 'pushButtonClickPackage')
        parser = packaging_widget.wait_select_single('Ubuntu::Internal::ClickRunChecksParser')

        sigwatch = parser.watch_signal("finished()")
        click_package_pushbutton.slots.click()
        self.assertThat(lambda: sigwatch.was_emitted, Eventually(Equals(True)))

        """ Check the error type if there was any error during the package creation """
        validation_groupbox = packaging_widget.wait_select_single('QGroupBox', objectName = 'groupBoxValidate')
        errorinfo_groupbox = validation_groupbox.wait_select_single('QGroupBox', objectName = 'groupBoxErrorInfo')
        errortype_label = errorinfo_groupbox.wait_select_single('QLabel', objectName = 'labelErrorType')
        self.assertThat(errortype_label.text, Equals(""))

    def test_x86_emulator_creation(self):
        """ Change to the Devices mode and click on the add new emulator button """
        kbd = Keyboard.create("X11")
        kbd.press_and_release('Ctrl+9')
        devices_quickview = self.ide.wait_select_single('QQuickView', source='file:///usr/share/qtcreator/ubuntu/qml/devicespage.qml')
        add_emulator_button = devices_quickview.select_single('Button', text='Add Emulator')
        add_emulator_button.visible.wait_for(True)
        """ The simple click_object() moves the pointer to the center of the button, but in some environment the responsive area of the button is smaller """
        self.pointing_device.move(add_emulator_button.globalRect.x + add_emulator_button.width - 1, add_emulator_button.globalRect.y + add_emulator_button.height - 1)
        self.pointing_device.click()
        config_emulator_dialog = devices_quickview.wait_select_single('Dialog', title='Create emulator')
        emulatorname_textfield = config_emulator_dialog.wait_select_single('TextField', placeholderText = 'Emulator name')
        self.pointing_device.click_object(emulatorname_textfield)
        kbd = Keyboard.create("X11")
        kbd.type("TestX86Emulator", delay=0.1)
        create_button = config_emulator_dialog.wait_select_single('Button', text = 'Create')
        self.pointing_device.click_object(create_button)
        """ Wait the emulator creation to finish """
        devices_ubuntulistview = devices_quickview.wait_select_single('UbuntuListView', objectName = 'devicesList')
        while True:
            if(devices_ubuntulistview.visible): break;
            sleep(1)
        emulator_listitem = devices_ubuntulistview.wait_select_single('Standard', text = 'TestX86Emulator')

    def test_x86_emulator_start(self):
        """ Change to the Devices mode, select the TestX86Emulator and deploy it """
        kbd = Keyboard.create("X11")
        kbd.press_and_release('Ctrl+9')
        devices_quickview = self.ide.wait_select_single('QQuickView', source='file:///usr/share/qtcreator/ubuntu/qml/devicespage.qml')
        devices_ubuntulistview = devices_quickview.wait_select_single('UbuntuListView', objectName = 'devicesList')
        while True:
            if(devices_ubuntulistview.visible): break;
            sleep(1)
        emulator_listitem = devices_ubuntulistview.wait_select_single('Standard', text = 'TestX86Emulator')
        self.pointing_device.click_object(emulator_listitem)
        # TODO: continue with capturing the started emulator en evaluate the status

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
