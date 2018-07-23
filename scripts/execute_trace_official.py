#!/usr/bin/python3
import os
import sys
import subprocess
import argparse
import re
import json
import time
import tqdm
import io
import base64
import numpy as np

# get it from https://pypi.org/project/selenium/
assert os.path.isfile('geckodriver')
os.environ['PATH'] += ':.'

import selenium
import selenium.webdriver
import selenium.webdriver.firefox.options


def parse_result(text):
    successful = 'Success' in text

    mo = re.compile(r'Commands:\s+(\d+)$\s*Energy:\s+(\d+)$', re.M).search(text)
    commands = int(mo.groups()[0])
    energy = int(mo.groups()[1])

    res = {
        'consumed_commands': commands,
        'energy': energy,
        'engine_name': 'official execute trace',
        'successful': successful,
        'msg': text
    }
    return res

'''
Success::
Time:      1398
Commands:  1398
Energy:    335111596
ClockTime: 580ms
'''

'''
Failure::
Halted with missing filled coordinates (425 = |{(1,9,11),(1,10,10),(1,10,11),(2,9,11),(2,9,12),...}|)
Halted with excess filled coordinates (188 = |{(7,1,9),(7,2,8),(7,3,8),(7,4,8),(7,5,7),...}|)

Time: 855
Commands: 855
Energy: 204790024
Harmonics: Low
#Full: 322
Active Bots: 
'''


def check(checkbox):
    if not checkbox.is_selected():
        checkbox.click()

def uncheck(checkbox):
    if checkbox.is_selected():
        checkbox.click()

class OfficialExecuteTrace(object):
    def __init__(self, headless=True):
        options = selenium.webdriver.firefox.options.Options()
        if headless:
            options.add_argument('--headless')

        self.browser = selenium.webdriver.Firefox(
            firefox_options=options)
        self.browser.set_window_position(0, 0)
        self.browser.set_window_size(700, 1000)
        self.browser.get('https://icfpcontest2018.github.io/full/exec-trace-novis.html')
        self.browser.execute_script("document.body.style.zoom='75%'")
        self.browser.execute_script("return document.getElementsByTagName('header')[0].remove()")

    def process_file(self, source_model_path, target_model_path, trace_path):
        source_empty = self.browser.find_element_by_id('srcModelEmpty')

        if source_model_path is None:
            check(source_empty)
        else:
            uncheck(source_empty)

            source = self.browser.find_element_by_id('srcModelFileIn')
            source.clear()
            source.send_keys(os.path.abspath(source_model_path))

        target_empty = self.browser.find_element_by_id('tgtModelEmpty')
        if target_model_path is None:
            check(target_empty)
        else:
            uncheck(target_empty)
            target = self.browser.find_element_by_id('tgtModelFileIn')
            target.clear()
            target.send_keys(os.path.abspath(target_model_path))

        trace = self.browser.find_element_by_id('traceFileIn')
        trace.clear()
        trace.send_keys(os.path.abspath(trace_path))

        self.browser.find_element_by_xpath("//select[@id='stepsPerFrame']/option[text()='4000']").click()

        self.browser.find_element_by_id('execTrace').click()

        while True:
            stdout = self.browser.find_element_by_id('stdout')
            if 'Failure' in stdout.text or 'Success' in stdout.text:
                break
            time.sleep(0.5)

        return stdout.text

    def __del__(self):
        self.browser.quit()
        pass

def test():
    executer = OfficialExecuteTrace()
    text = executer.process_file(
        '../data/problemsF/FD001_src.mdl',
        None,
        '../data/dfltTracesF/FD001.nbt')
    print(parse_result(text))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--model', default=None)
    parser.add_argument('--src-model', default=None)
    parser.add_argument('--trace', required=True)
    parser.add_argument('--info')

    args = parser.parse_args()

    assert args.model is not None or args.src_model is not None

    executer = OfficialExecuteTrace()
    text = executer.process_file(
        args.src_model,
        args.model,
        args.trace)
    j = parse_result(text)

    print(j['msg'])
    if args.info is not None and os.path.isdir(os.path.dirname(args.info)):
        with open(args.info, 'w') as fo:
            json.dump(j, fo)

if __name__ == '__main__':
    #test()
    main()

# vim: set si et sw=4 ts=4 sts=4:

