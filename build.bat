@echo off

set opts=-FC -GR- -EHa- -nologo -Zi
set code=%cd%
pushd build
cl %opts% %code%\src\zeus_linux.c -FeDungeonMaster
popd
