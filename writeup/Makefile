DOC=writeup
FIGURES=ecdf-fp-1-stack.pdf ecdf-serv-1-stack.pdf maxdepth.pdf random.pdf ecdf-serv-delta.pdf btype.pdf


all: $(DOC).pdf

$(FIGURES): plot.R
	./plot.R

$(DOC).pdf: $(DOC).tex $(FIGURES)
	pdflatex -file-line-error -interaction=nonstopmode $(DOC) 
	pdflatex -file-line-error -interaction=nonstopmode $(DOC) 

clean:
	rm $(DOC).aux $(DOC).log $(DOC).out $(DOC).pdf $(FIGURES)
