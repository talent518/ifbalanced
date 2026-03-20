# ifbalanced
Network interface balanced(Supported for TCP or UDP).

### compile
```sh
make
```
### install
```sh
make install
```
### usage 1
```sh
LD_PRELOAD=$PWD/libifbalanced.so IFBALANCED_FILE=$PWD/ifbalanced.conf command [argument...]
```
### usage 2
```sh
ifbalanced command [argument...]
```
### syslog
```sh
sudo tail -f /var/log/syslog
```
### test
```sh
make test
```
### debug
```sh
CFLAGS=-DUSE_GDEBUG make clean test
```
### environment variables
* IFBALANCED_LEVEL: set ifbalanced klog level(error: 3, info: 6)
* IFBALANCED_FILE: set ifbalanced config file(default: "/etc/ifbalanced.conf")

### config file command
* **empty line**
* **comment**: Starts with a # character, which may be preceded by a white space character
* **public** <interface>:<localaddr>
  * interface(string): network card interface name
  * localaddr(string): ipv4 address
* **private** <interface>:<localaddr>/<prefixlen>
  * interface(string): network card interface name
  * localaddr(string): ipv4 address
  * prefixlen(integer): if is 255.255.255.0 then 24

### modify config file /etc/sysctl.conf
```
net.ipv4.conf.all.rp_filter=0
net.ipv4.conf.default.rp_filter=0
```
