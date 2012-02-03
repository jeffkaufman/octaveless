OUT="definitions.h"

DISCLAIMER="""\
/* DON'T edit this file: it is autogenerated
 *
 * Instead, make changes to generate_defs.py and then run
 *
 *   python generate_defs.py
 *
 */
"""

BASSES = ["C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B"]

"""

increments:
0   .00  .24  .54  .22
1   .02  .26  .52  .20
2   .04  .28  .50  .18
3   .06  .30  .48  .16
4   .08  .32  .46  .14
5   .10  .34  .44  .12
6   .12  .36  .42  .10
7   .14  .38  .40  .08
8   .16  .40  .38  .06
9   .18  .42  .36  .04
10  .20  .44  .34  .02
11  .22  .46  .32  .00

"""

def fraction(bass_n, octave):
    root = [0.00, 0.24, 0.54, 0.22][octave]

    if octave in [0,1]:
        root += 0.02*bass_n
    else:
        root -= 0.02*bass_n

    return root

for bass_n, bass in enumerate(BASSES):
    s = sum(fraction(bass_n, octave) for octave in [0,1,2,3])
    print bass, s

def start():
    with open(OUT, "w") as outf:
        w = outf.write
        w(DISCLAIMER)

        for n in range(128):
            f = 440*(2**((n-69.0)/12))
            w("#define NOTE_%s %s\n" % (n, f))

        for octave in [0,1,2,3,4,5,6,7]:
            for bass_n, bass in enumerate(BASSES):
                note_n = 12 + octave*12 + bass_n
                w("#define NOTE_%s%s %s\n" % (bass, octave, note_n))

if __name__ == "__main__":
    start()
