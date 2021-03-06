#!/usr/bin/env python3
# via https://github.com/richardtsai/homework
import subprocess
import os.path
import uuid
import sys
import tempfile

testCases = [
        ('basetest.asm', 'basetest.mc'),
        ('label.asm', 'label.mc'),
        ('comments.asm', 'comments.mc'),
        ('negoffset.asm', 'negoffset.mc'),
        ('largeprogram.asm', 'largeprogram.mc'),
        ('example.asm', 'example.mc'),
        ('32bitfill.asm', '32bitfill.mc'),
        ('duplilabel.asm', None),
        ('invalidfields.asm', None),
        ('invalidins1.asm', None),
        ('invalidins2.asm', None),
        ('invalidoffset1.asm', None),
        ('invalidoffset2.asm', None),
        ('invalidregister1.asm', None),
        ('invalidregister2.asm', None),
        ('invalidregister3.asm', None),
        ('undeflabel.asm', None),
        ]

class WrongOutput(Exception):
    def __init__(self, lineno, exp, rel):
        self.lineno = lineno
        self.exp = exp
        self.rel = rel

    def __str__(self):
        return ('Line %d:\nExpectancy: %s\nReality:    %s'
                % (self.lineno, self.exp.strip(), self.rel.strip()))

class ErrorUndeteced(Exception):
    def __str__(self):
        return 'Failed to detect an error'

def getTempFileName():
    return os.path.join(tempDirObj.name, uuid.uuid4().hex)

def testOutput(infile, outfile):
    tempFileName = getTempFileName()

    if subprocess.call([assembler, infile, tempFileName],
            stderr = subprocess.DEVNULL):
        raise WrongOutput(-1, '', 'The program signals an error.')

    try:
        fobj1 = open(outfile, 'r')
        lines1 = fobj1.readlines()
        fobj2 = open(tempFileName, 'r')
        lines2 = fobj2.readlines()

        if len(lines1) != len(lines2):
            raise WrongOutput(-1,
                    str(len(lines1)) + ' lines', str(len(lines2)) + ' lines')

        for lineno, (l1, l2) in enumerate(zip(lines1, lines2)):
            if l1 != l2:
                raise WrongOutput(lineno, l1, l2)
    finally:
        fobj1.close()
        fobj2.close()

def testErrorsDetection(infile):
    if not subprocess.call([assembler, infile, getTempFileName()],
            stderr = subprocess.DEVNULL):
        raise ErrorUndeteced

def main():
    if len(sys.argv) < 3:
        print('Usage: python3 autotest.py [assembler] [path to test data]')
        return

    global assembler
    assembler = sys.argv[1]
    global testDataPath
    testDataPath = sys.argv[2]
    global tempDirObj
    tempDirObj = tempfile.TemporaryDirectory()

    print('LC2K Assembler Tests')
    failureCount = 0

    for infile, outfile in testCases:
        try:
            if outfile is None:
                testErrorsDetection(os.path.join(testDataPath, infile))
            else:
                testOutput(os.path.join(testDataPath, infile),
                        os.path.join(testDataPath, outfile))

        except (WrongOutput, ErrorUndeteced) as e:
            print('---\nTest Case: %s ... Fail!' % infile)
            print(e)
            failureCount += 1
        else:
            print('---\nTest Case: %s ... Pass!' % infile)

    print('---\nTesting finish!')
    print('Success:', len(testCases) - failureCount)
    print('Failure:', failureCount)
    print('Total:', len(testCases))

if __name__ == '__main__':
    main()
