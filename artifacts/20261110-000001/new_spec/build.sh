# sudo apt-get install fonts-inconsolata
# sudo fc-cache -fv
# sudo apt-get install pandoc texlive-latex-base texlive-fonts-recommended texlive-extra-utils texlive-latex-extra texlive-xetex
pandoc *.md --pdf-engine=xelatex -V "monofont:Inconsolata" -t latex -o output.pdf
