#!/usr/bin/python3
import os
import sys
import subprocess
import argparse
import re
import multiprocessing
import tqdm

REPO_DIR = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
#default_model_dir = os.path.join(REPO_DIR, 'data', 'problems') # lightling
default_model_dir = os.path.join(REPO_DIR, 'data', 'problemsF') # full
default_trace_output_dir = 'tmp-quick-traces'
default_info_output_dir = 'tmp-quick-info'
default_energy_output_dir = 'tmp-quick-energy'

def resolve_engine(engine_name):
    if os.path.isfile(engine_name):
        return engine_name
    engine_name = os.path.join(REPO_DIR, 'src', engine_name)
    if os.path.isfile(engine_name):
        return engine_name
    raise RuntimeError('engine {} not found!'.format(engine_name))

def job(args):
    cmds, engine_path, f = args
    cwd = os.path.dirname(engine_path)
    if len(cwd) == 0:
        cwd = None
    res = subprocess.run(cmds, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=cwd)
    return args, res

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('engine', help='engine executable')
    parser.add_argument('--jobs', type=int, default=2, help='number of processes')
    parser.add_argument('--model-dir', default=default_model_dir, help='directory containing models')
    parser.add_argument('--trace-output-dir', default=default_trace_output_dir, help='directory to store trace')
    parser.add_argument('--info-output-dir', default=default_info_output_dir, help='directory to store info')
    parser.add_argument('--energy-output-dir', default=default_energy_output_dir, help='directory to store energy')
    parser.add_argument('--type', default='ADR', help='A, D, R or combination of them')

    args = parser.parse_args()

    engine_path = resolve_engine(args.engine)
    assert os.path.isdir(args.model_dir)
    os.makedirs(args.trace_output_dir, exist_ok=True)
    os.makedirs(args.info_output_dir, exist_ok=True)

    n_failed = 0
    n_succeeded = 0
    error_results = []
    model_files = [f for f in os.listdir(args.model_dir) if os.path.splitext(f)[1].lower() == '.mdl']
    model_files.sort()
    commands = []
    for f in model_files:
        mo = re.match(r'(?P<base>[LF](?P<type>A|D|R)\d+)_(src|tgt)\.mdl', f)
        if mo is not None:
            base = mo.groupdict()['base']
            problem_type = mo.groupdict()['type']
            if problem_type not in args.type:
                continue

            src_model_path = os.path.abspath(os.path.join(args.model_dir, '{}_src.mdl'.format(base)))
            tgt_model_path = os.path.abspath(os.path.join(args.model_dir, '{}_tgt.mdl'.format(base)))
            trace_path = os.path.abspath(os.path.join(args.trace_output_dir, '{}.nbt'.format(base)))
            info_path = os.path.abspath(os.path.join(args.info_output_dir, '{}.json'.format(base)))
            energy_path = os.path.abspath(os.path.join(args.energy_output_dir, '{}.energy.json'.format(base)))

            if problem_type == 'A':
                assert os.path.isfile(tgt_model_path)
                cmds = [os.path.abspath(engine_path), '--model', tgt_model_path]
            if problem_type == 'D':
                assert os.path.isfile(src_model_path)
                cmds = [os.path.abspath(engine_path), '--src-model', src_model_path]
            if problem_type == 'R':
                assert os.path.isfile(src_model_path)
                assert os.path.isfile(tgt_model_path)
                cmds = [os.path.abspath(engine_path), '--model', tgt_model_path, '--src-model', src_model_path]

            cmds += ['--trace-output', trace_path, '--info', info_path, '--energy', energy_path]
            commands.append((cmds, engine_path, f))

    if args.jobs > 1:
        pool = multiprocessing.Pool(args.jobs)
        mapping = pool.imap
    else:
        mapping = map

    for (cmds, engine_path, f), res in tqdm.tqdm(mapping(job, commands), total=len(commands)):
        if res.returncode != 0:
            tqdm.tqdm.write('Fail: {}'.format(f))
            error_results.append((f, res))
            n_failed += 1
        else:
            n_succeeded += 1

    if n_failed > 0:
        print('FAILED MODELS:')
        for f, res in error_results:
            print('='*80)
            print('File: {}, returncode={}'.format(f, res.returncode))
            print(res.stdout.decode('utf-8'))
            print(res.stderr.decode('utf-8'))

    print('n_failed {} n_succeded {}'.format(n_failed, n_succeeded))
    
    if n_failed > 0:
        sys.exit(1)
    sys.exit(0)

# vim: set si et sw=4 ts=4 sts=4:
