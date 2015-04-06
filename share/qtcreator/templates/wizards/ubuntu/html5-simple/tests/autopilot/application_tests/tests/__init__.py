# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-

""" Autopilot tests """

import os
import subprocess

from selenium import webdriver
from selenium.webdriver.chrome.options import Options

import ubuntuuitoolkit as uitk
from autopilot import platform

from testtools.matchers import NotEquals, EndsWith
from autopilot.matchers import Eventually

from autopilot.testcase import AutopilotTestCase

CURRENT_ARCHITECTURE = subprocess.check_output(
    ["dpkg-architecture", "-qDEB_HOST_MULTIARCH"]).strip()
CHROMEDRIVER_EXEC_PATH = \
    "/usr/lib/{}/oxide-qt/chromedriver".format(
        CURRENT_ARCHITECTURE.decode("utf-8"))
DEFAULT_WEBVIEW_INSPECTOR_IP = '127.0.0.1'
DEFAULT_WEBVIEW_INSPECTOR_PORT = 9221


class HTML5TestCaseBase(AutopilotTestCase):
    def setUp(self):
        self.driver = None
        self.app_proxy = None
        super(HTML5TestCaseBase, self).setUp()

    def tearDown(self):
        print ('tearDown')
        if self.driver:
            self.page.close()
            self.page.quit()
        # XXX: This should not be there but AP hangs
        # if we dont extra force the process to be killed
        # (although AP already tries to kill part of its teardown)
        if platform.model() == 'Desktop' \
           and self.app_proxy \
           and self.app_proxy.process:
            self.app_proxy.process.kill()
        super(HTML5TestCaseBase, self).tearDown()

    def get_webview(self):
        return self.app_proxy.select_single("UbuntuWebView02")

    def launch_webdriver(self, ip, port):
        options = Options()
        options.binary_location = ''
        options.debugger_address = '{}:{}'.format(ip, port)

        self.driver = webdriver.Chrome(
            executable_path=CHROMEDRIVER_EXEC_PATH,
            chrome_options=options)
        self.assertThat(self.driver, NotEquals(None))

    @property
    def page(self):
        return self.driver

    def launch_html5_app_inline(self, args):
        return self.launch_test_application(
            'ubuntu-html5-app-launcher',
            *args,
            emulator_base=uitk.UbuntuUIToolkitCustomProxyObjectBase)

    def launch_html5_app(
            self,
            envvars={},
            ip=DEFAULT_WEBVIEW_INSPECTOR_IP,
            port=DEFAULT_WEBVIEW_INSPECTOR_PORT):

        # setup devtools environment
        self.patch_environment(
            'UBUNTU_WEBVIEW_DEVTOOLS_HOST',
            ip)
        self.patch_environment(
            'UBUNTU_WEBVIEW_DEVTOOLS_PORT',
            str(port))

        if envvars:
            for envvar_key in envvars:
                self.patch_environment(envvar_key, envvars[envvar_key])

        self.app_proxy = self.launch_html5_app_inline(
            ['--www={}/{}'.format(
                os.path.dirname(
                    os.path.realpath(__file__)),
                '../../../../www'), '--inspector'])
        self.wait_for_app()

        self.launch_webdriver(ip, port)

    def wait_for_app(self):
        self.assertThat(self.app_proxy, NotEquals(None))
        webview = self.get_webview()
        self.assertThat(
            lambda: webview.url,
            Eventually(EndsWith("/index.html")))
