import sys
import re

WIDTH = 80
HEADER_CHAR = '='
TITLE_CHAR = '-'
HEADER_SUFFIX = "<span class=right><span id=toggle_dark onclick=toggle_dark()> turn the lights on </span> <a href=https://github.com/friedelschoen/fiss><img id=github alt=GitHub src=assets/github-mark.svg /></a></span>"

PREFIX = """<!doctype html>
<html lang=en>

<head>
  <title>Friedel's Initialization and Service Supervision</title>
  <meta charset=utf-8 />
  <meta name=viewport content='width=device-width,initial-scale=1' />
  <link rel=stylesheet href=assets/style.css />
  <script type=text/javascript src=assets/toggle-dark.js></script>
</head>

<body>
<div id=wrapper>"""

SUFFIX = """
</div>
</body>
</html>
"""

HEADER_TEMPLATE = "<span class=header><a class=title id=top href=#top>{text}</a><span class=right><span id=toggle_dark onclick=toggle_dark()> turn the lights on </span> <a href=https://github.com/friedelschoen/fiss><img id=github alt=GitHub src=assets/github-mark.svg /></a></span></span>"
TITLE_TEMPLATE = "<a class=title id={id} href=#{id}>{text}</a>"

def inline_convert(text):
    text = re.sub(r'\*(.+?)\*', r'\\fB\\fC\1\\fR', text)
    text = re.sub(r'_(.+?)_', r'\\fI\1\\fR', text)
#    text = re.sub(r'\[(.*?)\]\((.*?)\)', r'<a href="\2">\1</a>', text)
    return text

in_list = False

#print(PREFIX)

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
        
#print(SUFFIX)