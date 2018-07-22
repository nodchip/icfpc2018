#!/usr/bin/env python
# --temp_trace_directory_path ../tmp/trace --temp_info_directory_path ../tmp/info --result_trace_directory_path ../result/trace --result_info_directory_path ../result/info
import argparse
import concurrent.futures
import json
import os
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description='update_result')
    parser.add_argument('--temp_trace_directory_path', required=True,
                        help='Directory path containing new trace files.')
    parser.add_argument('--temp_info_directory_path', required=True,
                        help='Directory path containing new info files.')
    parser.add_argument('--result_trace_directory_path', required=True,
                        help='Directory path containing current best trace files.')
    parser.add_argument('--result_info_directory_path', required=True,
                        help='Directory path containing current best trace files.')
    args = parser.parse_args()

    for model_name in sorted([os.path.splitext(f)[0]
                              for f
                              in os.listdir(args.temp_trace_directory_path)
                              if os.path.splitext(f)[1] == '.nbt']):
        temp_info_file_path = os.path.join(args.temp_info_directory_path, model_name + '.json')
        with open(temp_info_file_path, 'rt') as f:
            temp = json.load(f)

        if not temp['successful']:
            continue

        result_info_file_path = os.path.join(args.result_info_directory_path, model_name + '.json')
        with open(result_info_file_path, 'rt') as f:
            result = json.load(f)

        if result['energy'] <= temp['energy']:
            continue

        print('{} {} > {} ({:2f}%)'.format(model_name, result['energy'], temp['energy'], 100.0 * temp['energy'] / result['energy']))

        shutil.copy(temp_info_file_path, result_info_file_path)

        temp_trace_file_path = os.path.join(args.temp_trace_directory_path, model_name + '.nbt')
        result_trace_file_name = os.path.join(args.result_trace_directory_path, model_name + '.nbt')
        shutil.copy(temp_trace_file_path, result_trace_file_name)


if __name__ == '__main__':
	main()
