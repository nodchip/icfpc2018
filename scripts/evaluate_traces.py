import os
import sys
import subprocess
import tqdm
import json
import re

if __name__ == '__main__':
    model_dir = sys.argv[1]
    trace_dir = sys.argv[2]
    output_dir = 'evaluate_traces_output'
    use_official = len(sys.argv) > 3 and sys.argv[3] == 'official'

    os.makedirs(output_dir, exist_ok=True)
    inputs = []
    for f in sorted(os.listdir(trace_dir)):
        mo = re.match(r'F(.)(\d+).nbt', f)
        if mo is not None:
            problem_type, number = mo.groups()
            src_model_path = os.path.join(model_dir, 'F{}{}_src.mdl'.format(problem_type, number))
            tgt_model_path = os.path.join(model_dir, 'F{}{}_tgt.mdl'.format(problem_type, number))
            trace_path = os.path.join(trace_dir, os.path.splitext(f)[0] + '.nbt')
            info_path = os.path.join(output_dir, os.path.splitext(f)[0] + '.json')

            if problem_type == 'L' or problem_type == 'A': # lightling / full: assembly
                inputs.append((None, tgt_model_path, trace_path, info_path))
            if problem_type == 'D': # full: disassembly
                inputs.append((src_model_path, None, trace_path, info_path))
            if problem_type == 'R': # full: reassembly
                inputs.append((src_model_path, tgt_model_path, trace_path, info_path))

    n_failed = 0
    n_succeeded = 0
    if use_official:
        import execute_trace_official
        executer = execute_trace_official.OfficialExecuteTrace()
        for src_model_path, tgt_model_path, trace_path, info_path in tqdm.tqdm(inputs):
            tqdm.tqdm.write(trace_path)
            text = executer.process_file(
                src_model_path,
                tgt_model_path,
                trace_path)
            j = execute_trace_official.parse_result(text)
            if not j['successful']:
                tqdm.tqdm.write(j['msg'])
            with open(info_path, 'w') as fo:
                json.dump(j, fo, indent=4)

    else:
        for src_model_path, tgt_model_path, trace_path, info_path in inputs:
            cmds = ['../src/evaluate']
            if src_model_path:
                cmds += ['--src-model', src_model_path]
            if tgt_model_path:
                cmds += ['--model', tgt_model_path]
            cmds += ['--trace', trace_path]
            cmds += ['--info', info_path]
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
