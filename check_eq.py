# check whether two files are equal

import sys

def check_eq(file1, file2):
    with open(file1, 'r') as f1:
        with open(file2, 'r') as f2:
            for line1, line2 in zip(f1, f2):
                if line1 != line2:
                    return False
    return True

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: python check_eq.py file1 file2')
        sys.exit(1)
    file1 = sys.argv[1]
    file2 = sys.argv[2]
    if check_eq(file1, file2):
        print('Files are equal')
    else:
        print('Files are different')