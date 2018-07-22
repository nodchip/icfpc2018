import numpy as np
import pandas as pd
import multiprocessing
import tqdm
import os
import glob

def job(p):
    name = os.path.basename(p)
    arr = load_model(p)
    R = arr.shape[0]
    
    bbox = bbox_size(arr)
    cap = np.product(bbox)
    fill = np.count_nonzero(arr)

    return {
        'name': name,
        'R': R,

        'voxels': R**3,
        'fill': fill,

        'bbox_x': bbox[0],
        'bbox_y': bbox[2],
        'bbox_z': bbox[1],

        'bbox_ratio': cap / R**3,
        'fill_ratio': fill / R**3,
        'fill_bbox_ratio': fill / cap,
    }

def main():
    parse()
    analyze()

def analyze():
    df = pd.read_csv('problem_size.csv')
    print(df)

def parse():
    rows = []

    files = list(glob.glob('../data/problemsF/*.mdl'))
    with multiprocessing.Pool(6) as pool:
        for row in tqdm.tqdm(pool.imap(job, files), total=len(files)):
            rows.append(row)

    df = pd.DataFrame(rows)
    df.to_csv('problem_size.csv')

def bbox_size(arr):
    x = arr.max(axis=2).max(axis=1)
    y = arr.max(axis=2).max(axis=0)
    z = arr.max(axis=0).max(axis=0)
    def minmax(v):
        minidx, maxidx = 0, 0
        for i in range(len(v)):
            if v[i] > 0:
                minidx = i
                break
        for i in range(len(v) - 1, -1, -1):
            if v[i] > 0:
                maxidx = i
                break
        return maxidx - minidx

    return (minmax(x), minmax(y), minmax(z))


def load_model(p):
    with open(p, 'rb') as f:
        buf = f.read()
        R = buf[0]
        buf = np.frombuffer(buf, dtype=np.uint8, offset=1)

    arr = np.unpackbits(buf)[:R**3].reshape(R, R, R)
    return arr

if __name__ == '__main__':
    main()
