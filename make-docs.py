import sys

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

in_code = False
in_list = False

print(PREFIX)

for line in sys.stdin:
    line = line.strip()
    
    # is control
    if line.startswith("@header"):
        _, text = line.split(" ", 1)
        print(f"<span class=header><a class=title id=top href=#top>{text}</a>{HEADER_SUFFIX}</span>")
        print(HEADER_CHAR * WIDTH)
    elif line.startswith("@title"):
        _, id, text = line.split(" ", 2)
        print(f"<a class=title id={id} href=#{id}>{text}</a>")     
        print(TITLE_CHAR * WIDTH)
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
        sys.stdout.write("<ul>\n<li>")
        in_list = True
    elif line.startswith("@endlist"):
        sys.stdout.write("</li></ul>")
        in_list = False
    
    elif in_code:
        padding = WIDTH - 4 - len(line)
        if in_list:
            padding -= 2
        if padding < 0:
            padding = 0
        print('| ' + line + ' ' * padding + ' |')
    elif line.endswith('~'):
        print(line[:-1])
    elif line:
        sys.stdout.write(line + ' ') 
    elif in_list: # is empty but in line
        sys.stdout.write("</li>\n<li>")
    else: # is empty
        print()
        
print(SUFFIX)