import sys, re
from markdown_it import MarkdownIt
from pathlib import Path

src, out_html = sys.argv[1], sys.argv[2]
src_path = Path(src)
text = src_path.read_text(encoding='utf-8')

md = MarkdownIt('commonmark', {'html': True}).enable('table').enable('strikethrough')
body = md.render(text)

IMG_P = re.compile(r'<p>\s*(<img[^>]*alt="([^"]*)"[^>]*>)\s*</p>')


def _figure(m):
    img_tag, alt = m.group(1), m.group(2)
    cls = 'shot' if alt.startswith('界面') else 'uml'
    return '<figure class="%s">%s<figcaption>%s</figcaption></figure>' % (cls, img_tag, alt)


body = IMG_P.sub(_figure, body)

base_url = src_path.parent.as_uri() + '/'

CSS = """
@page { size: A4; margin: 17mm 15mm 15mm 15mm; }
* { box-sizing: border-box; }
body { font-family: "Microsoft YaHei", "SimSun", sans-serif; font-size: 10.5pt; line-height: 1.72;
       color: #1f2330; margin: 0; }
h1 { font-size: 19pt; text-align: center; margin: 0 0 4pt; letter-spacing: .5pt; }
h2 { font-size: 13.5pt; margin: 16pt 0 7pt; padding-bottom: 3pt; border-bottom: 1.2px solid #d0d4dd;
     page-break-after: avoid; }
h3 { font-size: 11.5pt; margin: 12pt 0 5pt; page-break-after: avoid; }
h2 + p, h3 + p { margin-top: 3pt; }
p { margin: 5pt 0; }
hr { border: none; border-top: 1px solid #e6e8ee; margin: 12pt 0; }
table { width: 100%; border-collapse: collapse; font-size: 8.8pt; margin: 7pt 0; line-height: 1.5; }
th, td { border: 1px solid #d8dce4; padding: 4pt 6pt; vertical-align: top; text-align: left; }
th { background: #f2f4f8; font-weight: 600; white-space: nowrap; }
td:first-child, th:first-child { width: 16%; }
tr { page-break-inside: avoid; }
ol, ul { margin: 5pt 0; padding-left: 20pt; }
li { margin: 2pt 0; }
code { font-family: "Consolas", "Microsoft YaHei", monospace; background: #f4f5f8;
       border-radius: 3px; padding: 0 3px; font-size: 9pt; }
blockquote { margin: 6pt 0; padding: 3pt 10pt; border-left: 3px solid #c7d2fe;
             background: #f8f9ff; color: #4b5563; font-size: 9.5pt; }
blockquote p { margin: 2pt 0; }
img { max-width: 100%; }
figure { margin: 8pt 0; page-break-inside: avoid; text-align: center; }
figcaption { font-size: 8.5pt; color: #6b7280; text-align: center; margin-top: 3pt; }
figure.uml img { max-width: 100%; max-height: 200mm; }
figure.shot { width: 48.6%; display: inline-block; vertical-align: top; margin: 0 0 9pt 0; }
figure.shot img { width: 100%; display: block; border: 1px solid #e6e8ee; }
"""

html = f"""<!DOCTYPE html>
<html lang="zh-CN"><head><meta charset="utf-8">
<title>校园二手交易平台 软件设计报告</title>
<base href="{base_url}">
<style>{CSS}</style></head>
<body>
{body}
</body></html>"""

Path(out_html).write_text(html, encoding='utf-8')
print("html written:", out_html, "| figures:", body.count('<figure'))
