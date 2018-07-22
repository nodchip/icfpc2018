#!/usr/bin/env python
import argparse
import concurrent.futures
import json
import os
import shutil
import subprocess
import sys


INVALID_ENERGY = 1e100


def main():
    parser = argparse.ArgumentParser(description='update_result')
    parser.add_argument('--default_info_directory_path', required=True,
                        help='Directory path containing info files for default traces.')
    parser.add_argument('--info_directory_path_base', required=True,
                        help='Parent directory path containing info files.')
    parser.add_argument('--engines', required=True,
                        help='Space-separated engine names. ex) "stupid_engine stupid_engine_v2 stupid_bbox_engine"')
    parser.add_argument('--input_model_directory_path', required=True,
                        help='Directory path containing input model files.')
    parser.add_argument('--output_html_file_path', required=True,
                        help='Directory path containing input model files.')
    args = parser.parse_args()

    engines = args.engines.split()

    with open(args.output_html_file_path, 'wt') as f:
        print('''<html>
<head>
<title>Engine comparison result</title>
</head>
<body>
<table border="1" cellspacing="0" cellpadding="0">
<tr>
<th>model name</th>
<th>default</th>
''', file=f)
        for engine in engines:
            print('<th>{}</th>'.format(engine), file=f)
        print('</tr>'.format(engine), file=f)

        info_directory_paths = list()
        info_directory_paths.append(args.default_info_directory_path)
        for engine in engines:
            info_directory_paths.append(os.path.join(args.info_directory_path_base, engine))

        model_titles = sorted(list({f[:f.index('_')]
                                    for f
                                    in os.listdir(args.input_model_directory_path)
                                    if os.path.splitext(f)[1] == '.mdl'}))
        for model_title in model_titles:
            print('<tr align="right">', file=f)
            print('<td>{}</td>'.format(model_title), file=f)
            energies = list()
            best_energy = 1e100
            for info_directory_path in info_directory_paths:
                info_file_path = os.path.join(info_directory_path, model_title + '.json')
                with open(info_file_path, 'rt') as json_file:
                    info = json.load(json_file)
                successful = info['successful']
                if not successful:
                    energies.append(INVALID_ENERGY)

                energy = info['energy']
                energies.append(energy)
                if best_energy > energy:
                    best_energy = energy
            default_energy = energies[0]

            for energy in energies:
                if energy == INVALID_ENERGY:
                    print('<td"></td>', file=f)

                if default_energy != best_energy:
                    color_density = int(255 * (default_energy - energy) / (default_energy - best_energy))
                    color_density = max(0, color_density)
                else:
                    color_density = 0

                print('<td bgcolor="#{0:02x}ff{0:02x}">{1}</td>'.format(255 - color_density, energy), file=f)
            print('</tr>', file=f)
        print('''</table>
</body>
</html>''', file=f)


if __name__ == '__main__':
	main()
