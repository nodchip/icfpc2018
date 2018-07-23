#!/usr/bin/env python
# --binary_file_path ../src/stupid_engine.exe --input_model_directory_path ../data/problemsF --output_trace_file_directory_path ../tmp/trace --output_info_file_directory_path ../tmp/info --output_energy_file_directory_path ../tmp/energy --jobs 4
import argparse
import concurrent.futures
import json
import os
import shutil
import subprocess
import sys
import threading
import traceback

import execute_trace_official


class ExecuteTraceOfficialPool:
    def __init__(self):
        self._pool = list()
        self._pool_lock = threading.Lock()

    def borrow_executer(self):
        with self._pool_lock:
            if self._pool:
                return self._pool.pop()
            return execute_trace_official.OfficialExecuteTrace()

    def return_executor(self, executor):
        with self._pool_lock:
            self._pool.append(executor)


def convert(args, model_title, binary_file_name, execute_trace_official_pool):
    input_src_model_file_path = os.path.join(args.input_model_directory_path, model_title + '_src.mdl')
    input_tgt_model_file_path = os.path.join(args.input_model_directory_path, model_title + '_tgt.mdl')
    output_trace_file_path = os.path.join(args.output_trace_file_parent_directory_path, binary_file_name, model_title + '.nbt')
    output_info_file_path = os.path.join(args.output_info_file_parent_directory_path, binary_file_name, model_title + '.json')
    output_energy_file_path = os.path.join(args.output_energy_file_parent_directory_path, binary_file_name, model_title + '.json')
    binary_file_path = os.path.join(args.binary_directory_path, binary_file_name)
    command = [binary_file_path,
               '--engine', args.engine_name,
               '--info', output_info_file_path,
               '--trace-output', output_trace_file_path,
               '--energy', output_energy_file_path,
               ]
    if os.path.isfile(input_src_model_file_path):
        command.extend(['--src-model', input_src_model_file_path])
    if os.path.isfile(input_tgt_model_file_path):
        command.extend(['--model', input_tgt_model_file_path])
    print(command)
    try:
        completed_process = subprocess.run(command, timeout=args.timeout_sec, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    except subprocess.TimeoutExpired as e:
        print(e)
        return subprocess.CompletedProcess(command, -1)

    if args.verify_with_official_executor:
        try:
            executor = execute_trace_official_pool.borrow_executer()

            source_model_path = None
            if os.path.isfile(input_src_model_file_path):
                source_model_path = input_src_model_file_path

            target_model_path = None
            if os.path.isfile(input_tgt_model_file_path):
                target_model_path = input_tgt_model_file_path

            text = executor.process_file(
                source_model_path,
                target_model_path,
                output_trace_file_path)
            official_json = execute_trace_official.parse_result(text)
            with open(output_info_file_path, 'rt') as f:
                my_json = json.load(f)

            if official_json['successful'] != my_json['successful']:
                stderr = '''official_json['successful'] != my_json['successful']
official_json['successful']={}
my_json['successful']={}'''.format(official_json['successful'], my_json['successful'])
                return subprocess.CompletedProcess(command, -1, stderr=stderr)

            if official_json['energy'] != my_json['energy']:
                stderr = '''official_json['energy'] != my_json['energy']
official_json['energy']={}
my_json['energy']={}'''.format(official_json['energy'], my_json['energy'])
                return subprocess.CompletedProcess(command, -1, stderr=stderr)
            execute_trace_official_pool.return_executor(executor)
        except Exception:
            return subprocess.CompletedProcess(command, -1, stderr=traceback.format_exc())

    return completed_process


def recreate_directory(path):
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path, exist_ok=True)


def main():
    parser = argparse.ArgumentParser(description='model_to_trace')
    parser.add_argument('--binary_file_names', required=True,
                        help='Space-separated binary file names to convert a model file (.mdl) to a trace file (.nbt).')
    parser.add_argument('--binary_directory_path', required=True,
                        help='Directory path containing binary files.')
    parser.add_argument('--jobs', type=int, default=1,
                        help='Number of jobs to execute at once.')
    parser.add_argument('--input_model_directory_path', required=True,
                        help='Directory path containing input model files.')
    parser.add_argument('--output_trace_file_parent_directory_path', required=True,
                        help='Output directory path that trace files are written into. The output directory is re-created if exists.')
    parser.add_argument('--output_info_file_parent_directory_path', required=True,
                        help='Output directory path that json files are written into. The output directory is re-created if exists.')
    parser.add_argument('--output_energy_file_parent_directory_path', required=True,
                        help='Output directory path that json files are written into. The output directory is re-created if exists.')
    parser.add_argument('--timeout_sec', type=int, default=60,
                        help='Timeout of each execution in seconds. ex) 60')
    parser.add_argument('--engine_name', default='default',
                        help='Engine name. ex) default')
    parser.add_argument('--verify_with_official_executor', action='store_true',
                        help='Use the official executor to verify the traces.')
    args = parser.parse_args()

    model_titles = {f[:f.index('_')]
                    for f
                    in os.listdir(args.input_model_directory_path)
                    if os.path.splitext(f)[1] == '.mdl'}
    binary_file_names = args.binary_file_names.split()
    for binary_file_name in binary_file_names:
        recreate_directory(os.path.join(args.output_trace_file_parent_directory_path, binary_file_name))
        recreate_directory(os.path.join(args.output_info_file_parent_directory_path, binary_file_name))
        recreate_directory(os.path.join(args.output_energy_file_parent_directory_path, binary_file_name))

    if args.verify_with_official_executor:
        OFFICIAL_EXECUTE_TRACE = execute_trace_official.OfficialExecuteTrace()

    execute_trace_official_pool = ExecuteTraceOfficialPool()
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = list()

        for model_title in model_titles:
            for binary_file_name in binary_file_names:
                futures.append(executor.submit(convert, args, model_title, binary_file_name, execute_trace_official_pool))
        for future in futures:
            completed_process = future.result()
            if completed_process.returncode:
                for future in futures:
                    future.cancel()
                print('!' * 80)
                print('Failed to execute an engine.')
                print('---------- args ----------')
                print('"{}"'.format('" "'.join(completed_process.args)))
                print('---------- returncode ----------')
                print(completed_process.returncode)
                if completed_process.stdout:
                    print('---------- stdout ----------')
                    print(completed_process.stdout.rstrip())
                if completed_process.stderr:
                    print('---------- stderr ----------')
                    print(completed_process.stderr.rstrip())
                sys.exit(1)


if __name__ == '__main__':
	main()
