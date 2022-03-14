import numpy as np
import tifffile as tiff
import sys
import os


def compare(ref_img, part_img, idd):
    print(idd)
    maxx = np.amax(ref_img)

    ref_marks  = np.unique(ref_img, return_counts=True)[1][1]
    # part_marks = np.unique(part_img, return_counts=True)[1][1]

    (h,w) = part_img.shape
    diff_img = ref_img[:h,:w] - part_img
    diff = np.unique(diff_img, return_counts=True)

    # diff_img = diff_img*255
    # tiff.imwrite(f'{idd}_diff.tiff', diff_img, photometric='minisblack')
    
    print('ref_mark | mark_false_neg(1->0) | mark_false_pos(0->1)')
    print(f'{ref_marks} | {diff[1][1]} | {diff[1][2]}')
    print(f'size {h*w}')


    # print(f'% diff total {100*diff/(h*w)}')
    # print(f'% diff mark  {100*missed_mark/total_area}')

    print('')

if __name__ == '__main__':
    # Load partitions tiles from text
    path = sys.argv[1]

    files = os.listdir(path)
    print(files)
    files.remove('ref.tiff')

    ref_img = tiff.imread(f'{path}/ref.tiff')
    for f in files:
        part_img = tiff.imread(f'{path}/{f}')
        idd = f.replace('m', '').replace('.tiff', '')
        compare(ref_img, part_img, idd)
