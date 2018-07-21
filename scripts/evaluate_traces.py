import os
import sys
import subprocess

if __name__ == '__main__':
    model_dir = sys.argv[1]
    trace_dir = sys.argv[2]
    output_dir = 'evaluate_traces_output'
    os.makedirs(output_dir, exist_ok=True)
    n_failed = 0
    n_succeeded = 0
    for f in os.listdir(trace_dir):
        if os.path.splitext(f)[1].lower() == '.nbt':
            model_path = os.path.join(model_dir, os.path.splitext(f)[0] + '_tgt.mdl')
            trace_path = os.path.join(trace_dir, f)
            info_path = os.path.join(output_dir, os.path.splitext(f)[0] + '.json')

            cmds = ['../src/evaluate', '--model', model_path, '--trace', trace_path, '--info', info_path]
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
