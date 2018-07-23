import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os
import json
import argparse

def main():
    df = pd.read_csv('compare_engine.csv', header=0)
    show_plot = False

    file_paths = []

    pagesize = 40
    for typename in 'FA FD FR'.split():
        df_type = df[df.model_name.str.startswith(typename)]
        for n in range(0, len(df_type), pagesize):
            print(typename, n)
            path, fig = generate_images(df_type.iloc[n:n+pagesize], typename, n)
            file_paths.append(path)
            if not show_plot:
                plt.close(fig)

    if show_plot:
        plt.show()

    with open('compare_engine_visualized.html', 'w') as f:
        print('''<html>
<head>
<title>Engine comparison result</title>
</head>
<body>
''', file=f)
        for path in file_paths:
            print('<div><img src="./{}" /></div>'.format(path), file=f)
        print('''</body></html>''', file=f)


def generate_images(df, name, num):
    n_models = len(df)
    n_engines = len(df.columns)
    df = df.set_index('model_name')
    df = df.transpose()
    column_names = df.columns

    fig, ax = plt.subplots(1, 1, figsize=(20, 7), dpi=72)
    y_pos = np.arange(n_models)
    y_shift = 1.0 / (n_engines + 1)
    width = y_shift
    for i, (idx, row) in enumerate(df.iterrows()):
        if idx == 'default':
            continue
        values = [row[k] for k in column_names]
        ax.bar(y_pos + y_shift * i, values, width, align='center',
            label='{}: sum={:.3g}'.format(idx, np.sum(values)))
    ax.grid()
    ax.set_xticks(y_pos)
    ax.set_xticklabels(column_names, rotation=45)
    ax.set_yscale('log')
    ax.set_title('type:{} {}~'.format(name, num))
    fig.legend(loc='right', fontsize=9)
    fig.tight_layout()
    fig.subplots_adjust(right=0.85)
    path = 'compare_engine_visualized_{}_{:05d}.png'.format(name, num)
    fig.savefig(path)

    return path, fig

if __name__ == '__main__':
    main()


