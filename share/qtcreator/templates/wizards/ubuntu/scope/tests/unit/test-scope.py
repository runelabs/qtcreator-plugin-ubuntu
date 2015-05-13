#!/usr/bin/env python3

from scope_harness import *
from scope_harness.testing import *
import unittest, sys, os
from subprocess import Popen, PIPE

class AppsTest (ScopeHarnessTestCase):
    @classmethod
    def setUpClass(cls):
        cls.process = Popen(["/usr/bin/python3", FAKE_SERVER], stdout=PIPE)
        port = cls.process.stdout.readline().decode("utf-8").rstrip('\n')
        os.environ["NETWORK_SCOPE_APIROOT"] = "http://127.0.0.1:" + port

    @classmethod
    def tearDownClass(cls):
        cls.process.terminate()


    def start_harness(self):
        self.harness = ScopeHarness.new_from_scope_list(Parameters([SCOPE_INI]))
        self.view = self.harness.results_view
        self.view.active_scope = SCOPE_NAME


    def test_surfacing_results(self):
        self.start_harness()
        self.view.search_query = ''

        match = CategoryListMatcher() \
            .has_exactly(2) \
            .mode(CategoryListMatcherMode.BY_ID) \
            .category(CategoryMatcher("current") \
                    .has_at_least(1) ) \
            .category(CategoryMatcher("forecast") \
                    .has_at_least(7) ) \
            .match(self.view.categories)
        self.assertMatchResult(match)


    def test_search_results(self):
        self.start_harness()
        self.view.search_query = 'Manchester,uk'

        match = CategoryListMatcher() \
            .has_exactly(2) \
            .mode(CategoryListMatcherMode.BY_ID) \
            .category(CategoryMatcher("current") \
                    .has_at_least(1) ) \
            .category(CategoryMatcher("forecast") \
                    .has_at_least(7) ) \
            .match(self.view.categories)
        self.assertMatchResult(match)


if __name__ == '__main__':
    SCOPE_NAME = sys.argv[1]
    SCOPE_INI = sys.argv[2]
    FAKE_SERVER = sys.argv[3]

    unittest.main(argv = sys.argv[:1])
