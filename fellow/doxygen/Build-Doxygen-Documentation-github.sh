#/bin/bash

echo "Removing prior latex directory"
rm latex -rf

echo "Running doxygen"
doxygen Doxyfile > WinFellow-doxygen.log 2>&1

echo "Changing to latex directory"
cd latex

echo "Running pdflatex etc."
pdflatex refman.tex > ../WinFellow-pdflatex1.log 2>&1
makeindex refman.idx  > ../WinFellow-makeindex.log 2>&1
pdflatex refman.tex > ../WinFellow-pdflatex2.log 2>&1

echo "Done running pdflatex etc."

cd ..

rm WinFellow-doxygen.pdf.bak
mv WinFellow-doxygen.pdf WinFellow-doxygen.pdf.bak
cp latex/refman.pdf ./WinFellow-doxygen.pdf
