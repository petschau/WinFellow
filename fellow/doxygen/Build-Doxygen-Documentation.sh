#/bin/bash
rm latex -rf

doxygen Doxyfile > WinFellow-doxygen.log

cd latex

pdflatex refman.tex
makeindex refman.idx
pdflatex refman.tex

cd ..

rm WinFellow-doxygen.pdf.bak
mv WinFellow-doxygen.pdf WinFellow-doxygen.pdf.bak
cp latex/refman.pdf ./WinFellow-doxygen.pdf

rm latex -rf
rm *.tmp