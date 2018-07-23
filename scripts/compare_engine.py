#!/usr/bin/env python
import argparse
import concurrent.futures
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import json
import os
import shutil
import subprocess
import sys


INVALID_ENERGY = 1e100
RANKING_COLORS = [
    'aqua',
    'lime',
    'yellow',
    ]


def main():
    parser = argparse.ArgumentParser(description='update_result')
    parser.add_argument('--default_info_directory_path', required=True,
                        help='Directory path containing info files for default traces.')
    parser.add_argument('--byhand_info_directory_path', required=False,
                        help='Directory path containing info files for hand written traces.',
                        default='../data/by_hand/info')
    parser.add_argument('--info_directory_path_base', required=True,
                        help='Parent directory path containing info files.')
    parser.add_argument('--engines', required=True,
                        help='Space-separated engine names. ex) "stupid_engine stupid_engine_v2 stupid_bbox_engine"')
    parser.add_argument('--input_model_directory_path', required=True,
                        help='Directory path containing input model files.')
    parser.add_argument('--output_html_file_path', required=True,
                        help='Directory path containing input model files.')
    parser.add_argument('--output_csv_file_path', default='compare_engine.csv',
                        help='CSV output path.')
    args = parser.parse_args()

    engines = args.engines.split()

    headers = []
    rows = []

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
<th>registered</th>
''', file=f)
        headers.append('model_name')
        headers.append('default')
        headers.append('registered')
        for engine in engines:
            print('<th>{}</th>'.format(engine), file=f)
            headers.append(engine)
        print('</tr>'.format(engine), file=f)
        headers.append('best')

        info_directory_paths = list()
        info_directory_paths.append(args.default_info_directory_path)
        if args.byhand_info_directory_path:
            info_directory_paths.append(args.byhand_info_directory_path)
        for engine in engines:
            info_directory_paths.append(os.path.join(args.info_directory_path_base, engine))

        model_titles = sorted(list({f[:f.index('_')]
                                    for f
                                    in os.listdir(args.input_model_directory_path)
                                    if os.path.splitext(f)[1] == '.mdl'}))
        for model_title in model_titles:
            row = []
            print('<tr align="right">', file=f)
            print('<td>{}</td>'.format(model_title), file=f)
            row.append(model_title)
            energies = list()
            best_energy = 1e100
            for info_directory_path in info_directory_paths:
                info_file_path = os.path.join(info_directory_path, model_title + '.json')
                successful = False
                try:
                    with open(info_file_path, 'rt') as json_file:
                        info = json.load(json_file)
                    successful = info['successful']
                except IOError:
                    pass

                if not successful:
                    energies.append(INVALID_ENERGY)
                    continue

                energy = info['energy']
                energies.append(energy)
                if best_energy > energy:
                    best_energy = energy
            default_energy = energies[0]

            energy_to_ranking = {v: k for k, v in reversed(enumerate(sorted(energies)))}

            for energy in energies:
                if energy == INVALID_ENERGY:
                    print('<td></td>', file=f)
                    row.append(np.nan)
                    continue

                rank = energy_to_ranking[energy]
                if default_energy < energy:
                    color = 'red'
                elif rank < len(RANKING_COLORS):
                    color = RANKING_COLORS[rank]
                else:
                    color = 'transparent'

                print('<td bgcolor="{0}">{1}</td>'.format(color, energy), file=f)
                row.append(energy)
            row.append(best_energy)
            rows.append(row)
            print('</tr>', file=f)
        print('''</table>
</body>
</html>''', file=f)

    df = pd.DataFrame(rows, columns=headers)
    df.to_csv(args.output_csv_file_path, index=False)

if __name__ == '__main__':
	main()
