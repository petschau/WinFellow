@echo off
del latex /S /Q
rmdir latex

doxygen Doxyfile > WinFellow-doxygen.log

cd latex

pdflatex refman.tex
makeindex refman.idx
pdflatex refman.tex

cd ..

taskkill /IM AcroRd32.exe
taskkill /IM Acrobat.exe

del  WinFellow-doxygen.pdf.bak
ren  WinFellow-doxygen.pdf WinFellow-doxygen.pdf.bak
copy latex\refman.pdf .\WinFellow-doxygen.pdf

del latex /S /Q
rmdir latex
del *.tmp /Q

pause