import numpy as np
import tifffile as tiff
import sys
import os


def merge(path, partitions, height, width):
    out_img_np = np.zeros((height, width), dtype='uint8')
    for (img_id, xi, xo, yi, yo) in partitions:
        if os.path.isfile(f'{path}/tmp/t{img_id}-rtOut-0-0.tiff') == False:
            continue
        part = tiff.imread(f'{path}/tmp/t{img_id}-rtOut-0-0.tiff')
        print(f'file: {path}/tmp/t{img_id}-rtOut-0-0.tiff')
        print(f'(h,w): {(yo, xo)}')
        print(f'part shape: {part.shape}')
        out_img_np[yi:yi + yo, xi:xi + xo] = part[:yo, :xo]

    tiff.imwrite(f'{path}/merged.tiff', out_img_np, photometric='minisblack')


if __name__ == '__main__':
    # Load partitions tiles from text
    path = sys.argv[1]

    partitions = []
    i = 0
    with open(f'{path}/tiles.txt') as f:
        for l in f.readlines():
            l = l.replace('\n', '').replace(',', ':').split(':')
            partitions.append((i, int(l[0]), int(l[1]), int(l[2]), int(l[3])))
            i = i + 1

            print(partitions)

    xs = []
    ys = []
    height = 41965
    width = 34531
    # for p in partitions:
    #     x = (p[1], p[2])
    #     y = (p[3], p[4])

    #     # print(f'partition {x} {y}')

    #     if x not in xs:
    #         xs.append(x)
    #         width = width + x[1]

    #     if y not in ys:
    #         ys.append(y)
    #         height = height + y[1]

    print(f'height {height}')
    print(f'width {width}')

    merge(path, partitions, height, width)