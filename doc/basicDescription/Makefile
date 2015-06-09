description.pdf: description.tex runtime.tex scheduling.tex learning.tex related.tex
	pdflatex description
	bibtex description
	pdflatex description
	bibtex description
	pdflatex description

clean:
	rm -f *.log *.aux *.bbl *.blg *.dvi *.lot *.toc *.lof description.pdf
