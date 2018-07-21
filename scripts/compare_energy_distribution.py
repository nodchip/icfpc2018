import matplotlib
matplotlib.use('tkagg')
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os
import json
import argparse

column_names = \
    "Halt", "Wait", "Flip", "SMove", "LMove", \
    "FillVoid", "FillFull", \
    "VoidFull", "VoidVoid", \
    "GFillVoid", "GFillFull", \
    "GVoidFull", "GVoidVoid", \
    "Fission", "Fusion", \
    "HighHarmonics", "LowHarmonics", "Bots", 

def main():
    parser = argparse.ArgumentParser(description='compare energy distribution JSON files.')
    parser.add_argument('json', nargs='+')
    parser.add_argument('--quiet', action='store_true')
    args = parser.parse_args()

    rows = []
    for f in args.json:
        if os.path.isfile(f):
            with open(f, 'r') as fi:
                j = json.load(fi)
            j['File'] = f
            rows.append(j)
    df = pd.DataFrame(rows)
    df = df[list(('File', ) + column_names)]
    print(df)

    fig, ax = plt.subplots(1, 1, figsize=(5, 8))
    y_pos = np.arange(len(column_names))
    y_shift = 1.0 / (len(df) + 1)
    width = y_shift
    for i, row in df.iterrows():
        values = [row[k] for k in column_names]
        ax.barh(y_pos + y_shift * i, values, width, align='center', edgecolor='black',
            label='{}: sum={:.3g}'.format(row['File'], np.sum(values)))
    ax.set_yticks(y_pos)
    ax.set_yticklabels(column_names)
    ax.legend()
    ax.set_title('consumed energy')
    fig.tight_layout()
    fig.savefig('consumed_energy.png')
    plt.show()

if __name__ == '__main__':
    main()

