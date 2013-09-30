# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-

"""Ubuntu Touch App autopilot tests."""

import os

from autopilot import input, platform
from autopilot.matchers import Equals, Eventually
from ubuntuuitoolkit import base, emulators


def _get_module_include_path():
    return os.path.join(get_path_to_source_root(), 'modules')


def get_path_to_source_root():
    return os.path.abspath(
        os.path.join(
            os.path.dirname(__file__), '..', '..', '..', '..'))


class ClickAppTestCase(base.UbuntuUIToolkitAppTestCase):
    """Common test case that provides several useful methods for the tests."""

    package_id = '' # TODO
    app_name = '%DISPLAYNAME%'
    desktop_file_path = '' #TODO
    installed_app_qml_location = '' #TODO

    def setUp(self):
        super(ClickAppTestCase, self).setUp()
        self.pointing_device = input.Pointer(self.input_device_class.create())
        self.app_qml_source_location = self._get_app_qml_source_path()
        self.launch_application()

        self.assertThat(self.main_view.visible, Eventually(Equals(True)))

    def _get_app_qml_source_path(self):
        qml_file_name = '{0}.qml'.format(self.app_name)
        return os.path.join(self._get_path_to_app_source(), qml_file_name)

    def _get_path_to_app_source(self):
        return os.path.join(get_path_to_source_root(), self.app_name)

    def launch_application(self):
        # On the phablet, we can only run the installed application.
        if platform.model() == 'Desktop' and self._application_source_exists():
            self._launch_application_from_source()
        else:
            self._launch_installed_application()

    def _application_source_exists(self):
        return os.path.exists(self.app_qml_source_location)

    def _launch_application_from_source(self):
        self.app = self.launch_test_application(
            'qmlscene', '-I' + _get_module_include_path(),
            self.app_qml_source_location,
            '--desktop_file_hint={0}'.format(self.desktop_file_path),
            app_type='qt',
            emulator_base=emulators.UbuntuUIToolkitEmulatorBase)

    def _launch_installed_application(self):
        if platform.model() == 'Desktop':
            self.launch_installed_application_with_qmlscene()
        else:
            self.launch_installed_click_application()
        
    def _launch_installed_application_with_qmlscene(self):
        self.app = self.launch_test_application(
            'qmlscene', self.installed_app_qml_location,
            '--desktop_file_hint={0}'.format(self.desktop_file_path),
            app_type='qt',
            emulator_base=emulators.UbuntuUIToolkitEmulatorBase)

    def _launch_installed_click_application(self):
        self.app = self.launch_click_package(self.pacakge_id, self.app_name)
        
    @property
    def main_view(self):
        return self.app.select_single(emulators.MainView)    
