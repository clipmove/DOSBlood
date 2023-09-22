@echo off
call clean.bat
cd orig
wlib engine -+a.obj -+engine.obj -+cache1d.obj
copy engine.lib ..\
copy mmulti.obj ..\
cd ..