import argparse
import os
import shutil
import threading
import subprocess


def convert(args, input_model_file_name):
    input_model_file_path = os.path.join(args.input_model_directory_path, input_model_file_name)
    model_title = input_model_file_name[:input_model_file_name.index('_')]
    output_trace_file_path = os.path.join(args.output_trace_file_directory_path, model_title + '.nbt')
    output_json_file_path = os.path.join(args.output_info_file_directory_path, model_title + '.json')
    command = [args.binary_file_path, '--model', input_model_file_path,
               '--engine', args.engine_name,
               '--info', output_json_file_path,
               '--trace-output', output_trace_file_path]
    print(command)
    try:
        completed_process = subprocess.run(command, timeout=args.timeout_sec)
    except subprocess.TimeoutExpired as e:
        print(e)


def main():
    parser = argparse.ArgumentParser(description='model_to_trace')
    parser.add_argument('--binary_file_path', required=True,
                        help='Binary file path to convert a model file (.mdl) to a trace file (.nbt).')
    #parser.add_argument('--jobs', required=True, type=int,
    #                    help='Number of jobs to execute at once.')
    parser.add_argument('--input_model_directory_path', required=True,
                        help='Directory path containing input model files.')
    parser.add_argument('--output_trace_file_directory_path', required=True,
                        help='Output directory path that trace files are written into. The output directory is re-created if exists.')
    parser.add_argument('--output_info_file_directory_path', required=True,
                        help='Output directory path that json files are written into. The output directory is re-created if exists.')
    parser.add_argument('--timeout_sec', required=True, type=int,
                        help='Timeout of each execution in seconds. ex) 60')
    parser.add_argument('--engine_name', required=True,
                        help='Engine name. ex) default')
    args = parser.parse_args()

    shutil.rmtree(args.output_trace_file_directory_path, ignore_errors=True)
    os.makedirs(args.output_trace_file_directory_path, exist_ok=True)

    shutil.rmtree(args.output_info_file_directory_path, ignore_errors=True)
    os.makedirs(args.output_info_file_directory_path, exist_ok=True)

    for input_model_file_name in [f for f in os.listdir(args.input_model_directory_path) if os.path.splitext(f)[1] == '.mdl']:
        convert(args, input_model_file_name)


if __name__ == '__main__':
	main()
