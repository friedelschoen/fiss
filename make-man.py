import sys
import re

def inline_convert(text):
    text = re.sub(r'\*(.+?)\*', r'\\fB\\fC\1\\fR', text)
    text = re.sub(r'_(.+?)_', r'\\fI\1\\fR', text)
    return text

in_list = False

for line in sys.stdin:
    line = line.strip()
    
    # is control
    if line.startswith("@man"):
        _, text = line.split(" ", 1)
        print(".TH " + text)
    elif line.startswith("@header"):
        pass
    elif line.startswith("@title"):
        _, id, text = line.split(" ", 2)
        print(".SH " + text.upper())    
    elif line.startswith("@code"):
        pass
    elif line.startswith("@endcode"):
        pass
    elif line.startswith("@list"):
        in_list = True
    elif line.startswith("@endlist"):
        in_list = False

    elif in_list and line.endswith('~'):
        sys.stdout.write(inline_convert(line[:-1]) + '\n')
    elif line.endswith('~'):
        sys.stdout.write(inline_convert(line[:-1]) + '\n.PP\n')
    elif line:
        sys.stdout.write(inline_convert(line) + ' ') 
    elif in_list: # is empty but in line
        sys.stdout.write("\n.PP\n")
    else: # is empty
        sys.stdout.write('\n.PP\n')