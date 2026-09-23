./genTestFile.sh
rm -rf /tmp/myFifo
./build/ipcTransfer -f testFileOut -r
md5sum testFile testFileOut
