#!/usr/bin/python3
import os
import sys
import subprocess
import argparse
import re
import time
import tqdm
import PIL.Image
import io
import base64
import numpy as np
import selenium
import selenium.webdriver

REPO_DIR = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
default_model_dir = os.path.join(REPO_DIR, 'data', 'problemsF') # full

default_output_dir = 'tmp-grab_model_viewer'
os.environ['PATH'] += ':.'

# get it from https://pypi.org/project/selenium/
assert os.path.isfile('geckodriver')

def resolve_model(name):
    if os.path.isfile(name):
        yield name
    if os.path.isdir(name):
        for f in os.listdir(name):
            if f.endswith('.mdl'):
                yield os.path.join(name, f)
    for candidate in [name+'_src.mdl', name+'_tgt.mdl']:
        candidate = os.path.join(default_model_dir, candidate)
        if os.path.isfile(candidate):
            yield candidate


class OfficialVisualizer(object):
    def __init__(self):
        self.browser = selenium.webdriver.Firefox()
        self.browser.set_window_position(0, 0)
        self.browser.set_window_size(700, 1000)
        self.browser.get('https://icfpcontest2018.github.io/view-model.html')
        self.browser.execute_script("document.body.style.zoom='75%'")
        self.browser.execute_script("return document.getElementsByTagName('header')[0].remove()")
        self.browser.execute_script("return document.getElementById('view-model').remove()")

    def process_file(self, model_path, output_path):
        upload = self.browser.find_element_by_id('modelFileIn')
        upload.clear()
        upload.send_keys(os.path.abspath(model_path))

        canvas = self.browser.find_element_by_id('glcanvas')
        canvas.click()
        canvas.send_keys('a'*12 + 's'*12)

        if False:
            screenshot = self.browser.find_element_by_xpath("//div[@id='glcanvas_container']/button")
            screenshot.click()

        if True:
            #self.browser.save_screenshot(output_path)
            png = self.browser.get_screenshot_as_png()
            im = PIL.Image.open(io.BytesIO(png))

            location = canvas.location
            size = canvas.size

            left = location['x']
            top = location['y']
            right = location['x'] + size['width']
            bottom = location['y'] + size['height']

            im = im.crop((left, top, right, bottom))
            im.save(output_path)

        if False:
            # get the canvas as a PNG base64 string
            canvas_base64 = self.browser.execute_script("return arguments[0].toDataURL('image/png');", canvas)[len('data:image/png;base64,'):]
            canvas_png = base64.b64decode(canvas_base64)

            with open(output_path, 'wb') as fo:
                fo.write(canvas_png)

    def __del__(self):
        #self.browser.quit()
        pass

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('model', nargs='*', default=[default_model_dir], help='model files or dirs')
    parser.add_argument('--sleep', type=float, default=0.5, help='sleep between iteration (s)')
    parser.add_argument('--overwrite', action='store_true')

    args = parser.parse_args()

    model_files = []
    for m in args.model:
        model_files += list(resolve_model(m))

    os.makedirs(default_output_dir, exist_ok=True)

    print('models:')
    for m in model_files:
        print(m)
    print('{} models'.format(len(model_files)))

    automation = OfficialVisualizer()
    for m in tqdm.tqdm(model_files):
        out = os.path.join(default_output_dir, os.path.splitext(os.path.basename(m))[0] + '.png')
        automation.process_file(m, out)
        time.sleep(args.sleep)

if __name__ == '__main__':
    main()

# vim: set si et sw=4 ts=4 sts=4:
