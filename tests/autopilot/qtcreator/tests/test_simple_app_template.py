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

    def test_options(self):
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

    def test_plugins(self):
        sleep(1)
        action = self.ide.wait_select_single(
            'QAction', text='About &Plugins...'
        )
        action.slots.trigger()
        sleep(10)
