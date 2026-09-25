#
изменил размер очереди максимальный 
```sh
sudo sysctl -w kernel.msgmax=2097152
sudo sysctl -w kernel.msgmnb=8388608
```