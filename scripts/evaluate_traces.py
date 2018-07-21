import os
import sys
import subprocess
import re

if __name__ == '__main__':
    model_dir = sys.argv[1]
    trace_dir = sys.argv[2]
    output_dir = 'evaluate_traces_output'
    os.makedirs(output_dir, exist_ok=True)
    n_failed = 0
    n_succeeded = 0
    for f in os.listdir(trace_dir):
        mo = re.match(r'F(.)(\d+).nbt', f)
        if mo is not None:
            problem_type, number = mo.groups()
            src_model_path = os.path.join(model_dir, 'F{}{}_src.mdl'.format(problem_type, number))
            tgt_model_path = os.path.join(model_dir, 'F{}{}_tgt.mdl'.format(problem_type, number))
            trace_path = os.path.join(trace_dir, os.path.splitext(f)[0] + '.nbt')
            info_path = os.path.join(output_dir, os.path.splitext(f)[0] + '.json')

            if problem_type == 'L' or problem_type == 'A': # lightling / full: assembly
                assert os.path.isfile(tgt_model_path)
                cmds = ['../src/evaluate', '--model', tgt_model_path, '--trace', trace_path, '--info', info_path]
            if problem_type == 'D': # full: disassembly
                assert os.path.isfile(src_model_path)
                cmds = ['../src/evaluate', '--src-model', src_model_path, '--trace', trace_path, '--info', info_path]
            if problem_type == 'R': # full: reassembly
                assert os.path.isfile(src_model_path)
                assert os.path.isfile(tgt_model_path)
                cmds = ['../src/evaluate', '--src-model', src_model_path, '--model', tgt_model_path, '--trace', trace_path, '--info', info_path]

            print(cmds)
            res = subprocess.run(cmds, stdout=subprocess.PIPE)
            if res.returncode != 0:
                print('Error[{}]'.format(res.returncode))
                print(res.stdout)
                n_failed += 1
            else:
                n_succeeded += 1
    print('n_failed {} n_succeded {}'.format(n_failed, n_succeeded))
    if n_failed > 0:
        sys.exit(1)
    sys.exit(0)

# vim: set si et sw=4 ts=4 sts=4:
