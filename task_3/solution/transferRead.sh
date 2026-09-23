./genTestFile.sh
rm -rf /tmp/myFifo
./build/ipcTransfer -o testFileOut -r
md5sum testFile testFileOut
