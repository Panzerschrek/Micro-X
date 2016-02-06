rm Micro-X-upx.exe
/C/lnk/upx391w/upx.exe Micro-X.exe -o Micro-X-upx.exe -9 --brute --ultra-brute --best --no-align
/C/lnk/kkrunchy_023a/kkrunchy.exe Micro-X.exe --good --out Micro-X-kk.exe
echo upx >> size_log.txt
du Micro-X-upx.exe -b | cut -f 1 >> size_log.txt
echo kkrunchy >> size_log.txt
du Micro-X-kk.exe -b | cut -f 1 >> size_log.txt
date +%H:%M\ %e.%m.%Y >> size_log.txt
echo \
 >> size_log.txt
