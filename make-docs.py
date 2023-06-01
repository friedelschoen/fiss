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
    text = re.sub(r'\*(.+?)\*', r'<b>\1</b>', text)
    text = re.sub(r'_(.+?)_', r'<u>\1</u>', text)
    text = re.sub(r'\[(.*?)\]\((.*?)\)', r'<a href="\2">\1</a>', text)
    return text

in_code = False
in_list = False

print(PREFIX)

for line in sys.stdin:
    line = line.strip()
    
    # is control
    if line.startswith("@man"):
        pass
    elif line.startswith("@header"):
        _, text = line.split(" ", 1)
        print(HEADER_TEMPLATE.format(text=text))
        sys.stdout.write(HEADER_CHAR * WIDTH)
    elif line.startswith("@title"):
        _, id, text = line.split(" ", 2)
        print(TITLE_TEMPLATE.format(id=id, text=text))
        sys.stdout.write(TITLE_CHAR * WIDTH)
    elif line.startswith("@code"):
        width = WIDTH -2
        if in_list:
            width -= 2
        print('+' + '-' * width + '+')
        in_code = True
    elif line.startswith("@endcode"):
        width = WIDTH -2
        if in_list:
            width -= 2
        sys.stdout.write('+' + '-' * width + '+')
        in_code = False
    elif line.startswith("@list"):
        sys.stdout.write("<div class=list>* ")
        in_list = True
    elif line.startswith("@endlist"):
        sys.stdout.write("</div>")
        in_list = False
    elif line == '~':
        print()
        
    elif in_code:
        padding = WIDTH - 4 - len(line)
        if in_list:
            padding -= 2
        if padding < 0:
            padding = 0
        print('| ' + line + ' ' * padding + ' |')
    elif line.endswith('~'):
        print(inline_convert(line[:-1]))
    elif line:
        sys.stdout.write(inline_convert(line) + ' ') 
    elif in_list: # is empty but in line
        sys.stdout.write("</div>\n<div class=list>* ")
    else: # is empty
        print('\n')
        
print(SUFFIX)